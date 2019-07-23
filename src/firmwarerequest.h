#ifndef FIRMWAREREQUEST_H
#define FIRMWAREREQUEST_H
#include <QUrlQuery>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>
#include <QSslError>
#include <QFile>
#include <vector>
#define TMP_BUFFER_SIZE (1024)

class FirmwareRequest : public QObject
{
        Q_OBJECT
public:
    enum Result{
        Error,
        Success
    };

    FirmwareRequest();
    ~FirmwareRequest();
    void setUID(const QString &uid);
    void getResourceList();
    void getResource(QUrl url, QString file = QString(""), bool addUID = false, uint length=0, bool redirect = false);

signals:
    void progress(const QString& message, int progress);
    void done(const QString& message, FirmwareRequest::Result status, char* result, int length);

private slots:
    void managerFinished(QNetworkReply *reply);
    void onIgnoreSSLErrors(QNetworkReply* reply, const QList<QSslError> &errors);
    void onReadyRead();
    void onFinished();

private:
    QString* uid;
    QNetworkAccessManager *manager;
    uint lastProgress;
    //resetet on every request!!!
    void cleanup();
    QNetworkReply* reply = nullptr;
    QFile* targetFile = nullptr;
    std::vector<char>* buffer;
    char tmpBuffer[TMP_BUFFER_SIZE];
    QString targetFileName = "";
    uint knwonContentLength = 0;

    const QString TEXT_CHECKING_4_UPDATES = QString("Checking for updates.");
    const QString TEXT_SENDING_REQUEST = QString("Sending request.");
    const QString TEXT_CHECKING_RESONSE = QString("Checking errors.");
    const QString TEXT_PARSING_RESPONSE = QString("Parsing response.");
    const QString TEXT_INVALID_RESPNSE = QString("Invalid server response.");
    const QString TEXT_RESPNSE_RECIEVED = QString("Response recieved.");
    const QString TEXT_CAN_NOT_CREATE_FILE = QString("Can't Create File.");
    const QString TEXT_PROGRESS_DOWNLOADING = QString("Downloading %1%.");

    const QString connectionError = QString("Conenction error");
    const QString repositoryURL = QString("https://update.undergroundfpv.com/update.php");
    const QVariant clinetAgent = QVariant("NV14-update-tool");
    const QString queryUID = QString("deviceUID");
};

#endif // FIRMWAREREQUEST_H
