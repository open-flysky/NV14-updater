#ifndef DFU_MANAGER_H
#define DFU_MANAGER_H
#include <dfu.h>
#include <stm32mem.h>
#include <QObject>
#include <QTimer>
#include <QMutex>
#define OTP_ADDRESS     0x1FFF7800
#define UID_ADDRESS     0x1FFF79E0
#define UID_SIZE 12
#define OTP_SIZE 512

#define FIRMWARE_BASE_ADDRESS       0x08000000
#define UID_FIRMWARE_ADDRESS        0x081E0000
#define UID_FIRMWARE_SEC_BACKUP     0x081E1000

#define TRANSFER_SIZE (2048)

enum DetectionState {
    NotStarted,
    UidReadDone,
    FwPrepared,
    FwWriteDone,
    FirstSectorOverwritten,
    UidValid
};

enum DfuAltSettings {
  InternalFlash = 0,
  OTP = 2
};

const uint VID = 0x483;
const uint PID = 0xDF11;

class DFUManager : public QObject
{
    Q_OBJECT
public:
    explicit DFUManager(QObject *parent = nullptr);
    ~DFUManager();

private:
    DetectionState detectionState = NotStarted;

    QTimer* detectionTimer;

    uint8_t* firstSector = nullptr;
    uint8_t* uidBuffer = nullptr;

    QByteArray uidFirmware;

    struct libusb_device *dev;
    dfu_device_t* dfu_device;
    uint8_t buffer[TRANSFER_SIZE];
    bool IoActive = false;
    bool installDriver();
    void releaseDevice(dfu_device_t * dfuDevice, bool event);
    bool readData(dfu_device_t * dfuDevice, uint startAddress, uint length, uint8_t* resultBuffer, bool silent);
    bool read(dfu_device_t * dfuDevice, uint startAddress, uint length, uint8_t* resultBuffer, bool silent);
    bool writeData(dfu_device_t * dfuDevice, uint address, char* buffer, uint length, bool silent, bool startApp);
    bool write(dfu_device_t * dfuDevice, uint address, char* buffer, uint length, bool silent, bool startApp);
    bool validateUID(dfu_device_t * dfuDevice);
    const QString getWDIError(int errorCode);
signals:
    void validationStarted();
    void validationDone();
    void foundDevice(const QString& uid);
    void driverEvent(const QString& message);
    void lostDevice();
    void restartRequired();
    void dfuDone(const QString& message, bool success);
    void progress(const QString& message, int progress);
private slots:
    void onDetectionTick();
public slots:
    void start();
    void stop();
    void flash(uint address, char* buffer, uint length);

private:
    const QString TEXT_DRIVER_NO_USB = "No driverless USB devices were found.";
    const QString TEXT_DRIVER_WDI_LIST_ERROR = "wdi_create_list: error";
    const QString TEXT_DRIVER_FOUND_DEV = "Found USB device: %1 %2:%3 %4";
    const QString TEXT_DRIVER_PREP_ERROR = "Can not prepare device driver";
    const QString TEXT_DRIVER_INSTALLING = "Installing wdi driver with %1 at %2";
    const QString TEXT_DRIVER_INSTALLED = "WinUSB driver successfully installed";

    const QString TEXT_DRIVER_WDI_ERROR_IO = "Input/output error";
    const QString TEXT_DRIVER_WDI_ERROR_INVALID_PARAM = "Invalid parameter";
    const QString TEXT_DRIVER_WDI_ERROR_ACCESS = "Access denied (insufficient permissions)";
    const QString TEXT_DRIVER_WDI_ERROR_NO_DEVICE = "No such device (it may have been disconnected)";
    const QString TEXT_DRIVER_WDI_ERROR_NOT_FOUND = "Entity not found";
    const QString TEXT_DRIVER_WDI_ERROR_BUSY = "Resource busy, or API call already running";
    const QString TEXT_DRIVER_WDI_ERROR_TIMEOUT = "Operation timed out";
    const QString TEXT_DRIVER_WDI_ERROR_OVERFLOW = "Overflow";
    const QString TEXT_DRIVER_WDI_ERROR_PENDING_INSTALLATION = "Another installation is pending";
    const QString TEXT_DRIVER_WDI_ERROR_INTERRUPTED = "System call interrupted (perhaps due to signal)";
    const QString TEXT_DRIVER_WDI_ERROR_RESOURCE = "Could not acquire resource (Insufficient memory, etc)";
    const QString TEXT_DRIVER_WDI_ERROR_NOT_SUPPORTED = "Operation not supported or unimplemented on this platform";
    const QString TEXT_DRIVER_WDI_ERROR_EXISTS = "Entity already exists";
    const QString TEXT_DRIVER_WDI_ERROR_USER_CANCEL = "Cancelled by user";
    const QString TEXT_DRIVER_WDI_ERROR_NEEDS_ADMIN = "Couldn't run installer with required privileges";
    const QString TEXT_DRIVER_WDI_ERROR_WOW64 = "Attempted to run the 32 bit installer on 64 bit";
    const QString TEXT_DRIVER_WDI_ERROR_INF_SYNTAX = "Bad inf syntax";
    const QString TEXT_DRIVER_WDI_ERROR_CAT_MISSING = "Missing cat file";
    const QString TEXT_DRIVER_WDI_ERROR_UNSIGNED = "System policy prevents the installation of unsigned drivers";
    const QString TEXT_DRIVER_WDI_ERROR_OTHER = "Other error";

    const QString TEXT_INVALID_PARAMS = "Invalid parameters!";
    const QString TEXT_CLEARING_STARTED = "Clearing chip...";
    const QString TEXT_CLEARING_PAGE_AT = "Clearing page at 0x%1...";
    const QString TEXT_CLEARING_DONE = "Clearing chip done.";
    const QString TEXT_ERROR_CLEARING = "An error occurred while clearing chip!";

    const QString TEXT_BURNING_STARTED = "Burning firmware...";
    const QString TEXT_BURNING_FW_PROGRESS = "Burning at 0x%1 - %2%";
    const QString TEXT_BURNING_ERROR = "An error [%1] occurred while writting!";
    const QString TEXT_BURNING_OK = "Flash programmed";

    const QString TEXT_READINGG_STARTED = "Reading firmware...";
    const QString TEXT_READING_FW_PROGRESS = "Reading at 0x%1 - %2%";
    const QString TEXT_READING_ERROR = "An error [%1] occurred while reading!";
    const QString TEXT_READING_OK = "Flash programmed";
};

#endif // DFU_MANAGER_H
