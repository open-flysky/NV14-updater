#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "firmwarerequest.h"
#include <qmimedata.h>
#include <QDrag>
#include <QMessageBox>
#include <QFileInfo>
#include "aboutdialog.h"
#include <QSaveFile>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    firmware = nullptr;
    firmwareLength =0;
    validDFUDevice = false;
    ui->setupUi(this);
    ui->fwList->setVisible(false);
    appendStatus("NV14-Update tool started");
    qApp->installEventFilter(this);
    fwRequest = new FirmwareRequest();
    connect(fwRequest, SIGNAL (progress(const QString&, int)), this, SLOT (onProgress(const QString&, int)));
    connect(fwRequest, SIGNAL (done(const QString&, FirmwareRequest::Result, char*, int)), this, SLOT (onDone(const QString&, FirmwareRequest::Result, char*, int)));
    connect(ui->action, SIGNAL(released()), this, SLOT(actionTriggered()));
    connect(ui->fwList, SIGNAL(currentIndexChanged(int)), this, SLOT(onResoueceChanged(int)));

    dfu_manager.moveToThread(&dfu_thread);

    connect(&dfu_thread, SIGNAL(started()), &dfu_manager, SLOT(start()));
    connect(&dfu_thread, SIGNAL(finished()), &dfu_manager, SLOT(stop()));

    connect(&dfu_manager, SIGNAL(foundDevice(const QString&)), this, SLOT(foundDevice(const QString&)));
    connect(&dfu_manager, SIGNAL(lostDevice()), this, SLOT(lostDevice()));
    connect(&dfu_manager, SIGNAL(dfuDone(const QString&, bool)), this, SLOT(onDfuDone(const QString&, bool)));
    connect(&dfu_manager, SIGNAL(progress(const QString&, int)), this, SLOT(onProgress(const QString&, int)));
    connect(&dfu_manager, SIGNAL(restartRequired()), this, SLOT(restartRequired()));
    connect(&dfu_manager, SIGNAL(validationStarted()), this, SLOT(onValidationStarted()));
    connect(&dfu_manager, SIGNAL(validationDone()), this, SLOT(onValidationDone()));
    connect(&dfu_manager, SIGNAL(driverEvent(const QString&)), this, SLOT(onDriverEvent(const QString&)));


    connect(this, SIGNAL(doFlash(uint, char*, uint)), &dfu_manager, SLOT(flash(uint, char*, uint)));
    connect(this, SIGNAL(startUSB()), &dfu_manager, SLOT(start()));

    dfu_thread.start();
    setOperation(CheckForUpdates);
}

MainWindow::~MainWindow()
{
    disposed = true;
    delete ui;
}


bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if(event->type() == QEvent::MouseButtonRelease && obj == ui->labelHelp_2){
        AboutDialog about(this);
        about.exec();
    }
    return QObject::eventFilter(obj, event);
}


void MainWindow::appendStatus(const QString& message){
 ui->fwInfo->append("[" + QDateTime::currentDateTime().toString("hh:mm:ss") + "] " + message);
}

void MainWindow::onProgress(const QString& message, int progress){
    ui->progressBar->setValue(progress);
    appendStatus(message);
}

void MainWindow::setButton(const QString& message, const QString& style, QPixmap* icon, bool enabled){
    ui->action->setIcon(*icon);
    ui->action->setText(message);
    ui->action->setStyleSheet(style);
    ui->action->setEnabled(enabled);
}

void MainWindow::setOperationAfterTimeout(Operation nextOperation, int timeout){
    QTimer* timer = new QTimer;
    QObject::connect(timer, &QTimer::timeout, [this, nextOperation](){
        setOperation(nextOperation);
    });
    connect(timer, &QTimer::timeout, timer, &QTimer::deleteLater);
    timer->setSingleShot(true);
    timer->start(timeout);
}

void MainWindow::setError(Operation nextOperation){
    setButton("Error", styleRed, &img_error, false);
    setOperationAfterTimeout(nextOperation, 5000);
}

void MainWindow::onDone(const QString& message, FirmwareRequest::Result status, char* result, int length){
    ui->progressBar->setValue(100);


    appendStatus(message);
    if(status == FirmwareRequest::Error){
        setError(CheckForUpdates);
        return;
    }
    if(currentOperation == CheckForUpdates){
        QString error;
        remoteFiles.clear();
        ui->fwList->clear();
        if(!RemoteFileInfo::ParseResourceList(result, length, remoteFiles, error)){
            if(error.length() > 0) appendStatus(error);
            setError(CheckForUpdates);
            return;
        }
        ui->fwList->addItem("---SELECT---");
        foreach (const RemoteFileInfo& rfi, remoteFiles){
            ui->fwList->addItem(rfi.name);
        }
        setOperation(SelectResource);
    }
    else if (currentOperation == DownloadFirmware || currentOperation == DownloadArchive){
        RemoteFileInfo rfi = remoteFiles[remoteFileIndex()];
        appendStatus(message);
        appendStatus(Text_VALIDATIN_CHECKSUM);
        if(!rfi.isValid(result, length)){
           appendStatus(Text_INVALID_CHECKSUM);
           setError(SelectResource);
           return;
        }
        appendStatus(Text_VALID_CHECKSUM);
        switch(rfi.type){
        case RemoteFileInfo::ResourceType::Archive:
            setOperation(SelectResource);
            break;
        case RemoteFileInfo::ResourceType::Firmware:
            //create copy!
            firmware = result;
            firmwareLength = length;
            setOperation(UpdateTX);
            break;
        }
    }
}

void MainWindow::setOperation(Operation operation){
    currentOperation = operation;
    ui->fwList->setVisible(operation != CheckForUpdates);
    ui->fwList->setEnabled(operation < UpdateTX);
    switch(operation){
    case DetectTX:
        setButton(Text_DETECTING_TX, styleGreen, &img_wait, false);
        if(validDFUDevice) setOperation(BurnFirmware);
        break;
    case CheckForUpdates:
        ui->fwList->clear();
        setButton(Text_CHECKING, styleGreen, &img_wait, false);
        fwRequest->getResourceList();
        break;
    case SelectResource:
        setButton(Text_SELECT_RESOURCE, styleBlue, &img_reload, true);
        ui->fwList->setCurrentIndex(0);
        break;
    case DownloadFirmware:
        setButton(Text_DOWNLOAD_FW, styleBlue, &img_reload, true);
        break;
    case DownloadArchive:
        setButton(Text_DOWNLOAD_ARCHIVE, styleBlue, &img_reload, true);
        break;
    case UpdateTX:
        setButton(Text_UPDATE_TX, styleBlue, &img_reload, true);
        break;

    case BurnFirmware:
        setButton(Text_UPDATING_TX, styleBlue, &img_wait, false);
        if (firmwareLength == 0 || firmware== nullptr) {
            appendStatus("File is empty!");
            setError(SelectResource);
            return;
        }
        ui->progressBar->setValue(0);
        ui->progressBar->show();
        emit doFlash(0x8000000U, firmware, firmwareLength);
        break;
    case Done:
        setButton(Text_UPDATE_OK, styleBlue, &img_ok, false);
        setOperationAfterTimeout(SelectResource, 5000);
        break;

    default:
        break;
    }
}


void MainWindow::actionTriggered()
{

    if(currentOperation == DownloadArchive){
        RemoteFileInfo rfi = remoteFiles[remoteFileIndex()];
        QString defaultFilter("%1 files (*.%1)");
        defaultFilter = defaultFilter.arg(rfi.fileName.split(".").last());
        QString path = QFileDialog::getSaveFileName(this, tr("Save file as"), rfi.fileName, defaultFilter + ";;All files (*.*)", &defaultFilter);
        if(path.length()==0) return;
        setButton(Text_DOWNLOADING, styleGreen, &img_wait, false);
        fwRequest->getResource(rfi.url, path, rfi.size);
        return;
    }

    switch(currentOperation){
    case DownloadFirmware:
    {
        QString path;
        RemoteFileInfo rfi = remoteFiles[remoteFileIndex()];
        setButton(Text_DOWNLOADING, styleGreen, &img_wait, false);
        fwRequest->getResource(rfi.url, path, rfi.size);
    }
        break;
    case UpdateTX:
    {
        if(validDFUDevice) setOperation(BurnFirmware);
        else setOperation(DetectTX);
    }
        break;
    default:
        break;
    }
}

uint MainWindow::remoteFileIndex(){
    return static_cast<uint>(ui->fwList->currentIndex() -1);
}

void MainWindow::onResoueceChanged(int index)
{
    if(index <= 0 || remoteFiles.empty() || remoteFiles.size() < static_cast<uint>(index)){
        //check is removing items
        if(currentOperation > SelectResource){
            setOperation(SelectResource);
        }
        return;
    }
    RemoteFileInfo rfi = remoteFiles[remoteFileIndex()];
    switch(rfi.type){
        case RemoteFileInfo::Archive:
            setOperation(DownloadArchive);
            break;
        case RemoteFileInfo::Firmware:
            setOperation(DownloadFirmware);
            break;
    }
}

void MainWindow::foundDevice(const QString& uid){
    if(disposed) return;
    validDFUDevice = true;
    appendStatus(QString("NV14 found"));
    //initial state
    if(currentOperation == CheckForUpdates){
        QFileInfo fi = QFileInfo("firmware.bin");
        if(fi.exists() && fi.isFile()){
            QFile file("firmware.bin");
            if (file.open(QIODevice::ReadOnly)){
                QByteArray blob = file.readAll();
                firmware = new char[blob.count()];
                memcpy(firmware, blob.data(), blob.count());
                firmwareLength = blob.count();
                setOperation(BurnFirmware);
            }
        }
    }
    else if(currentOperation == DetectTX) {
        setOperation(BurnFirmware);
    }
}
void MainWindow::lostDevice(){
    if(disposed) return;
    return;
    validDFUDevice = false;
    ui->action->setEnabled(false);
    //after done auto reload to detect
    if(currentOperation != Done) setOperation(DetectTX);
}

void MainWindow::onDfuDone(const QString& message, bool success){
    if(disposed) return;
    appendStatus(message);
    if(!success){
        setError(UpdateTX);
    }
    else{
        setOperation(Done);
    }
}
void MainWindow::onDriverEvent(const QString& message){
    if(currentOperation == DetectTX){
        setButton(Text_DRIVER_INSTALL, styleGreen, &img_wait, false);
        appendStatus(message);
        //if(!validDFUDevice)foundDevice("003300283437511");
    }
}

void MainWindow::closeEvent (QCloseEvent *event)
{
    QMessageBox::StandardButton resBtn = QMessageBox::question(
                this, QCoreApplication::applicationName(),
                currentOperation == BurnFirmware ? Text_ABORT_UPLOAD : Text_CLOSE,
                QMessageBox::Cancel | QMessageBox::No | QMessageBox::Yes,
                QMessageBox::No);
    if (resBtn != QMessageBox::Yes) {
        event->ignore();
    } else {
        //dfu_thread.quit();
        dfu_thread.exit();
        event->accept();
    }
}

void MainWindow::restartRequired(){
    QMessageBox::information(this, QCoreApplication::applicationName(), Text_RESTART, QMessageBox::Ok, QMessageBox::Ok);
}



