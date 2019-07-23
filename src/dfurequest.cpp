#include "dfurequest.h"

#include <QDebug>
#include <QFile>

#include <dfu.h>
#include <stm32mem.h>

#define RETRY_MAX   5

#if defined(__cplusplus)
extern "C" {
    #include <libusb-1.0/libusb.h>
    extern libusb_context *usbcontext;
}
#endif

#ifdef _MSC_VER
#include <libwdi.h>
#endif


DFUManager::DFUManager(QObject *parent) :
    QObject(parent)
{
    firstSector = new uint8_t[STM32_FIRST_SECTOR_SIZE];
    uidBuffer = new uint8_t[UID_SIZE];
    detectionTimer = new QTimer(this);
    detectionTimer->setInterval(5000);
    connect(detectionTimer, SIGNAL(timeout()), this, SLOT(onDetectionTick()));
    dfu_device = new dfu_device_t();
    dfu_device->handle = nullptr;
    dfu_device->interface = 0;
}

DFUManager::~DFUManager()
{
    delete[] firstSector;
    delete[] uidBuffer;
}
void DFUManager::releaseDevice(dfu_device_t * dfuDevice, bool event){
    //dfu_detach(dfu_device, 1000);
    if (dfuDevice->handle != nullptr) {
        try{
            libusb_release_interface(dfuDevice->handle, dfuDevice->interface);
            libusb_close(dfuDevice->handle);
        }
        catch(...){}
        dfuDevice->handle = nullptr;
    }
    if(event) emit lostDevice();
}
void DFUManager::start()
{
    onDetectionTick();
    detectionTimer->start();
}
void DFUManager::stop()
{
    releaseDevice(dfu_device, true);
    detectionTimer->stop();
}

const QString DFUManager::getWDIError(int errorCode){
    switch(errorCode){
#ifdef _MSC_VER
    case WDI_ERROR_IO:                  return TEXT_DRIVER_WDI_ERROR_IO;
    case WDI_ERROR_INVALID_PARAM:       return TEXT_DRIVER_WDI_ERROR_INVALID_PARAM;
    case WDI_ERROR_ACCESS:              return TEXT_DRIVER_WDI_ERROR_ACCESS;
    case WDI_ERROR_NO_DEVICE:           return TEXT_DRIVER_WDI_ERROR_NO_DEVICE;
    case WDI_ERROR_NOT_FOUND:           return TEXT_DRIVER_WDI_ERROR_NOT_FOUND;
    case WDI_ERROR_BUSY:                return TEXT_DRIVER_WDI_ERROR_BUSY;
    case WDI_ERROR_TIMEOUT:             return TEXT_DRIVER_WDI_ERROR_TIMEOUT;
    case WDI_ERROR_OVERFLOW:            return TEXT_DRIVER_WDI_ERROR_OVERFLOW;
    case WDI_ERROR_PENDING_INSTALLATION:return TEXT_DRIVER_WDI_ERROR_PENDING_INSTALLATION;
    case WDI_ERROR_INTERRUPTED:         return TEXT_DRIVER_WDI_ERROR_INTERRUPTED;
    case WDI_ERROR_RESOURCE:            return TEXT_DRIVER_WDI_ERROR_RESOURCE;
    case WDI_ERROR_NOT_SUPPORTED:       return TEXT_DRIVER_WDI_ERROR_NOT_SUPPORTED;
    case WDI_ERROR_EXISTS:              return TEXT_DRIVER_WDI_ERROR_EXISTS;
    case WDI_ERROR_USER_CANCEL:         return TEXT_DRIVER_WDI_ERROR_USER_CANCEL;
    case WDI_ERROR_NEEDS_ADMIN:         return TEXT_DRIVER_WDI_ERROR_NEEDS_ADMIN;
    case WDI_ERROR_WOW64:               return TEXT_DRIVER_WDI_ERROR_WOW64;
    case WDI_ERROR_INF_SYNTAX:          return TEXT_DRIVER_WDI_ERROR_INF_SYNTAX;
    case WDI_ERROR_CAT_MISSING:         return TEXT_DRIVER_WDI_ERROR_CAT_MISSING;
    case WDI_ERROR_UNSIGNED:            return TEXT_DRIVER_WDI_ERROR_UNSIGNED;
#endif
    default: return TEXT_DRIVER_WDI_ERROR_OTHER;
    }
}

bool DFUManager::installDriver(){
    bool hasUsbDev = true;
#ifdef _MSC_VER
    hasUsbDev = false;
    char const *path = "Drivers";
    char const *winUsb= "WinUSB";
    char const *libusbK = "libusbK";
    char const *INF_NAME= "NV14_dfu_driver.inf";
    struct wdi_device_info *device, *list;
    static struct wdi_options_create_list cl_options = { true, false, false };
    static struct wdi_options_prepare_driver pd_options = { WDI_WINUSB, nullptr, nullptr, false, false, nullptr, false };
    int result = wdi_create_list(&list, &cl_options);
    switch (result) {
    case WDI_SUCCESS:
        break;
    case WDI_ERROR_NO_DEVICE:
        emit driverEvent(TEXT_DRIVER_NO_USB);
        return false;
    default:
        emit driverEvent(TEXT_DRIVER_WDI_LIST_ERROR);
        return false;
    }
    for (device = list; device != nullptr; device = device->next) {
        if(device->vid != VID || device->pid != PID) continue;
        if((strcmp(device->driver, winUsb) == 0) || (strcmp(device->driver, libusbK) == 0)) {
            hasUsbDev = true;
            continue;
        }
        emit driverEvent(QString(TEXT_DRIVER_FOUND_DEV).arg(device->desc).arg(device->vid, 4,16).arg(device->pid, 4,16).arg(device->driver));
        result = wdi_prepare_driver(device, path, INF_NAME, &pd_options);
        if (result == WDI_SUCCESS) {
            emit  driverEvent(QString(TEXT_DRIVER_INSTALLING).arg(INF_NAME).arg(path));
            result = wdi_install_driver(device, path, INF_NAME, nullptr);
            if(result == WDI_SUCCESS){
                hasUsbDev = true;
                driverEvent(TEXT_DRIVER_INSTALLED);
            }
            else{
                emit driverEvent(getWDIError(result));
            }
        }
        else{
            emit driverEvent(getWDIError(result));
        }
    }
    wdi_destroy_list(list);
#endif
    return hasUsbDev;
}


void DFUManager::onDetectionTick()
{
    if(IoActive) return;
    bool usbDevExists = installDriver() && dfu_device_exists(usbcontext, VID, PID, 0,0) != 0;
    bool hasDevice = false;
    if (dfu_device->handle) {
        if(usbDevExists){
            return;
        }
        hasDevice = true;
        qDebug() << "Releasing Interface...";
        releaseDevice(dfu_device, true);
    }
    qDebug() << "Trying to find DFU devices...";
    dfu_device_t * dfuDevice = new dfu_device_t();
    dfuDevice->handle = nullptr;
    dfuDevice->interface = 0;

    libusb_device* usbLibDev = dfu_device_init(usbcontext, VID, PID, 0,0, dfuDevice, 1, 1, InternalFlash);

    if(usbLibDev == nullptr){
        qDebug() << "FATAL: No compatible device found!\n";
        emit lostDevice();
        return;
    }

    int state = dfu_get_state(dfuDevice);
    if((state < 0) || (state == STATE_APP_IDLE)) {
        qDebug() << "Resetting device in firmware upgrade mode...";
        releaseDevice(dfuDevice, hasDevice);
        return;
    }

    dev = usbLibDev;
    dfu_device->handle = dfuDevice->handle;
    dfu_device->interface =  dfuDevice->interface;
    emit foundDevice(QString(""));

    /*
    if(validateUID(dfuDevice)){
        QByteArray a = QByteArray(reinterpret_cast<char*>(uidBuffer), UID_SIZE);
        dev = usbLibDev;
        dfu_device->handle = dfuDevice->handle;
        dfu_device->interface =  dfuDevice->interface;
        emit foundDevice(QString(a.toHex()));
    }
    else{
        releaseDevice(dfuDevice, hasDevice);
    }
    */
}

//after each failure new detection will occur
bool DFUManager::validateUID(dfu_device_t * dfuDevice){
    if(detectionState == NotStarted || detectionState == FirstSectorOverwritten || detectionState == UidValid){
        emit validationStarted();
        memset(uidBuffer, 0xFF, UID_SIZE);
        if(!read(dfuDevice, UID_ADDRESS, UID_SIZE, uidBuffer, true)) return false;
        bool all00 = true;
        bool allFF = true;
        for(unsigned i=0; i < UID_SIZE; i+=4) {
            uint32_t val = *reinterpret_cast<uint32_t*>(uidBuffer +i);
            if(val != 0) all00 = false;
            if(val != 0xFFFFFFFF) allFF = false;
            //swap bytes - to get coorect representation - open tx uses little endian
            //toHex from Qt represent values byte by byte
            for(unsigned j=0; j < 2; j++){
                uint8_t byteVal = uidBuffer[i+j];
                uidBuffer[i+j] = uidBuffer[i+3-j];
                uidBuffer[i+3-j] = byteVal;
            }
        }

        if(!all00 && !allFF){
            emit validationDone();
            detectionState = UidValid;
            return true;
        }

        if(detectionState != FirstSectorOverwritten){
            detectionState = UidReadDone;
        }

    }
    if(detectionState < FwPrepared){
        memset(firstSector, 0xFF, STM32_FIRST_SECTOR_SIZE);
        if(!read(dfuDevice, FIRMWARE_BASE_ADDRESS, STM32_FIRST_SECTOR_SIZE, firstSector, true)) return false;
        QFile file(":/resources/UID.binary");
        if (!file.open(QIODevice::ReadOnly)) return false;
        uidFirmware = file.readAll();
        //push reset handler backup
        char* dataPtr = reinterpret_cast<char*>(firstSector);
        //create copy of 1st sector
        //reserve up to 4K
        int backupOffset = UID_FIRMWARE_SEC_BACKUP - UID_FIRMWARE_ADDRESS;
        while(uidFirmware.size() < backupOffset){
            uidFirmware.push_back(static_cast<char>(0xFF));
        }
        for(uint i = 0; i < STM32_FIRST_SECTOR_SIZE; i++){
            uidFirmware.push_back(dataPtr[i]);
        }
        //update reset handler address in 1st sector
        uint8_t * data = reinterpret_cast<uint8_t*>(uidFirmware.data());
        memcpy(firstSector, data, 8);
        detectionState = FwPrepared;
    }

    if(detectionState < FwWriteDone){
        if(!write(dfuDevice, UID_FIRMWARE_ADDRESS, uidFirmware.data(), static_cast<uint>(uidFirmware.length()), true, false)) return false;
        detectionState = FwWriteDone;
    }
    if(detectionState < FirstSectorOverwritten){
        if(!write(dfuDevice, FIRMWARE_BASE_ADDRESS, reinterpret_cast<char*>(firstSector), STM32_FIRST_SECTOR_SIZE, true, true)) return false;
        detectionState = FirstSectorOverwritten;
        emit validationDone();
    }
    return false;
}
bool DFUManager::read(dfu_device_t * dfuDevice, uint startAddress, uint length, uint8_t* resultBuffer, bool silent)
{
    IoActive = true;
    bool result = readData(dfuDevice, startAddress, length, resultBuffer, silent);
    IoActive = false;
    return result;
}
bool DFUManager::readData(dfu_device_t * dfuDevice, uint startAddress, uint length, uint8_t* resultBuffer, bool silent)
{
    int32_t status = UNSPECIFIED_ERROR;
    if(startAddress < 0x08000000 || length > 0x00200000) {
        if(!silent) emit dfuDone(TEXT_INVALID_PARAMS, false);
        return false;
    }
    uint32_t offset = 0;
    uint32_t bytes = 0;
    if((status = stm32_set_address_ptr(dfuDevice, startAddress))) {
        if(!silent) emit dfuDone(TEXT_READING_ERROR.arg(status), false);
        return false;
    }
    dfu_set_transaction_num(0);
    while( offset < length ) {
        bytes = length - offset;
        if(bytes > TRANSFER_SIZE) bytes = TRANSFER_SIZE;
        memset(buffer, 0xFF, TRANSFER_SIZE);
        int p = static_cast<int>(((offset*100U)/length));
        if(!silent) emit progress(TEXT_READING_FW_PROGRESS.arg(startAddress + offset, 8, 16, QLatin1Char('0')).arg(p), p);
        offset += bytes;
        int attempt = 0;
        do {
            status = stm32_read_block(dfuDevice, bytes, buffer);
            if (status == LIBUSB_ERROR_PIPE) {
                dfu_clear_status( dfuDevice );
            }
        } while ((status == LIBUSB_ERROR_PIPE) && (attempt++<RETRY_MAX));

        if(status) {
            if(!silent) emit dfuDone(TEXT_READING_ERROR.arg(status), false);
            return false;
        }
        memcpy(resultBuffer + (offset-bytes), buffer, bytes);
        if(!silent && offset == length) emit progress(TEXT_READING_FW_PROGRESS.arg(startAddress + offset, 8, 16, QLatin1Char('0')).arg(100), 100);
    }
    if(!silent) emit dfuDone(TEXT_READING_OK, true);
    return true;
}

void DFUManager::flash(uint startAddress, char* data, uint length)
{
    if(!write(dfu_device, startAddress, data, length, false, true)){
        releaseDevice(dfu_device, true);
    }
}

bool DFUManager::write(dfu_device_t * dfuDevice, uint startAddress, char* data, uint length, bool silent, bool startApp){
    IoActive = true;
    bool result = writeData(dfuDevice, startAddress, data, length, silent, startApp);
    IoActive = false;
    return result;
}

bool DFUManager::writeData(dfu_device_t * dfuDevice, uint startAddress, char* data, uint length, bool silent, bool startApp){
    int32_t status = UNSPECIFIED_ERROR;
    if(startAddress < 0x08000000 || length > 0x00200000) {
        if(!silent) emit dfuDone(TEXT_INVALID_PARAMS, false);
        return false;
    }
    uint lastPage = 1;
    int firstPage = -1;
    uint numberOfPages = sizeof(stm32_sector_addresses)-1;
    //find pages
    for(uint page = 0; page < numberOfPages ; page++){
        uint pageStart = stm32_sector_addresses[page];
        uint pageEnd =  stm32_sector_addresses[page+1]-1;

        if(pageStart >= startAddress + length) break; //page start after image
        if(pageEnd < startAddress) continue; //page ends before image
        if(firstPage == -1) firstPage = static_cast<int>(page);
        lastPage = page;
    }

    if(firstPage == -1){
        if(!silent) emit dfuDone(TEXT_INVALID_PARAMS, false);
        return false;
    }
    if(!silent) emit progress(TEXT_CLEARING_STARTED, 0);
    dfu_clear_status( dfuDevice );
    for(uint page = static_cast<uint>(firstPage); page <= lastPage ; page++){
        if(!silent) emit progress(TEXT_CLEARING_PAGE_AT.arg(stm32_sector_addresses[page], 8, 16, QLatin1Char('0')), 0);
        int attempt = 0;
        while(attempt++ < 3){
            status = stm32_page_erase(dfuDevice, stm32_sector_addresses[page], false);
            if(status == 0) break;
            dfu_clear_status( dfuDevice );
        }
        if(status) {
            if(!silent) emit dfuDone(TEXT_ERROR_CLEARING, false);
            return false;
        }
    }
    if(!silent) emit progress(TEXT_CLEARING_DONE, 0);
    if(!silent) emit progress(TEXT_BURNING_STARTED, 0);

    uint32_t offset = 0;
    uint32_t bytes;


    if((status = stm32_set_address_ptr(dfuDevice, startAddress))) {
        if(!silent) emit dfuDone(TEXT_BURNING_ERROR.arg(status), false);
        return false;
    }

    dfu_set_transaction_num( 2 );
    while( offset < length ) {
        memset(buffer, 0xFF, TRANSFER_SIZE);
        bytes = length - offset;
        if(bytes > TRANSFER_SIZE) bytes = TRANSFER_SIZE;
        memcpy(buffer, data + offset, bytes);
        offset += TRANSFER_SIZE;
        if( (status = stm32_write_block( dfuDevice, TRANSFER_SIZE, buffer )) ) {
            if(!silent) emit dfuDone(TEXT_BURNING_ERROR.arg(status), false);
            return false;
        }
        int p = static_cast<int>(((offset*100U)/length));
        if(!silent) emit progress(TEXT_BURNING_FW_PROGRESS.arg(startAddress + offset, 8, 16, QLatin1Char('0')).arg(p), p);
    }


    if(!silent) emit dfuDone(TEXT_BURNING_OK, true);
    if(startApp){
        emit restartRequired();
        stm32_set_address_ptr(dfuDevice, STM32_FLASH_OFFSET);
        stm32_start_app(dfuDevice, false, STM32_FLASH_OFFSET);
        releaseDevice(dfuDevice, true);
    }
    return true;
}


