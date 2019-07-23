#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QKeyEvent>
#include <QSettings>
#include <string>
#include <QToolTip>
#include <QDateTime>
#include <QTimer>
#include <QFileDialog>
#include <QThread>
#include <QCloseEvent>

#include "firmwarerequest.h"
#include "remotefileinfo.h"
#include <vector>
#include "dfurequest.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
    enum Operation {
        CheckForUpdates,
        SelectResource,
        DownloadFirmware,
        DownloadArchive,
        UpdateTX,
        DetectTX,
        BurnFirmware,
        Done,
        None
    };

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
signals:
    void doFlash(uint address, char* buffer, uint length);
    void startUSB();
protected:
    bool eventFilter(QObject *obj, QEvent *event);
    void closeEvent (QCloseEvent *event);
private slots:
    void onDone(const QString& message, FirmwareRequest::Result status, char* result, int length);
    void onResoueceChanged(int index);
    void actionTriggered();
    //DFU
    void foundDevice(const QString& uid);
    void lostDevice();
    void onValidationStarted();
    void onValidationDone();
    void onDriverEvent(const QString& message);
    void onDfuDone(const QString& message, bool success);
    void onProgress(const QString& message, int progress);
    void restartRequired();
private:
    uint remoteFileIndex();
    void appendStatus(const QString& message);
    void checkUID();

    void setError(Operation nextOperation);
    void setOperation(Operation operation);
    void setOperationAfterTimeout(Operation operation, int timeout);
    void setButton(const QString& message, const QString& style, QPixmap* icon, bool enabled);
    void fillFwList(QJsonDocument* result);
    QVariant GetFwInfo(QJsonValue& val);

    Operation currentOperation = DetectTX;
    Ui::MainWindow *ui;
    FirmwareRequest* fwRequest;
    DFUManager dfu_manager;

    bool validDFUDevice;
    char* firmware;
    uint firmwareLength;
    std::vector<RemoteFileInfo> remoteFiles;

    QThread dfu_thread;
    bool disposed = false;

    QPixmap img_error = QPixmap(QString::fromUtf8(":/resources/error.png"));
    QPixmap img_ok = QPixmap(QString::fromUtf8(":/resources/ok.png"));
    QPixmap img_help = QPixmap(QString::fromUtf8(":/resources/help.png"));
    QPixmap img_info = QPixmap(QString::fromUtf8(":/resources/info.png"));
    QPixmap img_reload = QPixmap(QString::fromUtf8(":/resources/reload.png"));
    QPixmap img_sd = QPixmap(QString::fromUtf8(":/resources/sd.png"));
    QPixmap img_wait = QPixmap(QString::fromUtf8(":/resources/wait.png"));
    QPixmap img_warning = QPixmap(QString::fromUtf8(":/resources/warning.png"));

    QString styleRed = "background-color: rgb(189, 33, 48); color: rgb(255, 255, 255); font: 75 12pt \"Courier New\"";
    QString styleGreen = "background-color: rgb(30, 126, 52); color: rgb(255, 255, 255); font: 75 12pt \"Courier New\"";
    QString styleOrange = "background-color: rgb(255, 193, 7); color: rgb(255, 255, 255); font: 75 12pt \"Courier New\"";
    QString styleBlue = "background-color: rgb(0, 88, 204); color: rgb(255, 255, 255);\nfont: 75 12pt \"Courier New\"";

    const QString UID_LIST_0 = "UID_LIST_0";
    const QString UID_LIST_1 = "UID_LIST_1";
    const QString UID_LIST_2 = "UID_LIST_2";
    const QString Text_VALIDATIN_CHECKSUM = "Validating checksum";
    const QString Text_INVALID_CHECKSUM = "Invalid checksum";
    const QString Text_CHECK_UPDATES = "Check for updates";
    const QString Text_SELECT_RESOURCE = "Select resource";
    const QString Text_DOWNLOAD_ARCHIVE = "Download archive";
    const QString Text_DOWNLOAD_FW = "Download firmware";
    const QString Text_UPDATE_TX = "Update TX";
    const QString Text_UPDATING_TX = "TX updating...";
    const QString Text_UPDATE_OK = "Update succeeded";
    const QString Text_DETECTING_TX = "TX detecting...";
    const QString Text_DOWNLOADING = "Downloading...";
    const QString Text_CHECKING = "Checking for updates...";

    const QString Text_DRIVER_INSTALL = "Driver is being installed..." ;

    const QString Text_VALIDATION_STARTED = "Unit validation running...";
    const QString Text_VALIDATION_RUNNING = "Don't disconnect!!!";
    const QString Text_VALIDATION_DONE = "Unit validation finished";


    const QString Text_ABORT_UPLOAD = "Aborting when firmware is being flashed may damage your unit!!!\r\nDo you really want to quit?";
    const QString Text_CLOSE = "Do you really want to quit?\n";
    const QString Text_RESTART = "Please restart your unit.\r\nRemove batteries if necessary.";
};

#endif // MAINWINDOW_H
