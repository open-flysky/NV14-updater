#include "firmwarerequest.h"
FirmwareRequest::FirmwareRequest()
{
    lastProgress = 0;
    targetFileName = "";
    knwonContentLength = 0;
    buffer = new std::vector<char>();
    this->manager = new QNetworkAccessManager();
    connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(managerFinished(QNetworkReply*)));
    connect(manager, SIGNAL(sslErrors(QNetworkReply*, const QList<QSslError>&)),this,SLOT(onIgnoreSSLErrors(QNetworkReply*, const QList<QSslError>&)));
}

FirmwareRequest::~FirmwareRequest()
{
    delete uid;
    delete manager;
    cleanup();
}
void FirmwareRequest::cleanup(){
    lastProgress = 0;
    if(reply != nullptr) {
        delete reply;
        reply = nullptr;
    }
    if(targetFile != nullptr) {
        if(targetFile->isOpen()) {
            targetFile->flush();
            targetFile->close();
        }
        delete targetFile;
        targetFile = nullptr;
    }
    if(buffer !=nullptr && buffer->size() > 0) {
        buffer->clear();
    }
}

void FirmwareRequest::setUID(const QString &uid){
    this->uid = new QString(uid);
}
void FirmwareRequest::onIgnoreSSLErrors(QNetworkReply* reply, const QList<QSslError> &errors){
    if(manager == nullptr) return;
    foreach (const QSslError &error, errors)
        qDebug() << "skip SSL error: " << qPrintable(error.errorString());
    reply->ignoreSslErrors(errors);
}


void FirmwareRequest::managerFinished(QNetworkReply *reply){
    if(manager == nullptr) return;
    QNetworkReply::NetworkError error = reply->error();
    int contentLength = reply->header(QNetworkRequest::ContentLengthHeader).toInt();
    int code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if(code >=300 && code < 400){
        QByteArray header = reply->rawHeader("location");
        QUrl url(QString::fromUtf8(header));
        if(targetFile != nullptr) {
            if(targetFile->isOpen()) {
                targetFile->seek(0);
            }
        }
        if(buffer != nullptr) buffer->clear();
        getResource(url, targetFileName, false, knwonContentLength, true);
        return;
    }


    contentLength = static_cast<int>(buffer->size());

    if(contentLength == 0 && buffer->size() == 0){
        QByteArray resp = reply->readAll();
        const unsigned char* begin = reinterpret_cast<unsigned char*>(resp.data());
        const unsigned char* end = begin + resp.length();
        buffer = new std::vector<char>( begin, end );
    }
    if(!error && code >=200 && code <=300) emit done(TEXT_RESPNSE_RECIEVED,  Result::Success, buffer->data(), contentLength);
    else emit done(reply->errorString(), Result::Error, nullptr, 0);
    //else emit done(connectionError, Result::Error, nullptr, 0);
}


void FirmwareRequest::onReadyRead(){
    uint contentLength = reply->header(QNetworkRequest::ContentLengthHeader).toUInt();
    if(contentLength == 0 && knwonContentLength > 0) {
        contentLength = knwonContentLength;
    }
    if(targetFileName.count() > 0) {
        targetFile = new QFile(targetFileName);
        try {
             targetFile->open(QFile::OpenModeFlag::ReadWrite);
        } catch (...) {
            emit done(TEXT_CAN_NOT_CREATE_FILE, Error, nullptr, 0);
            return;
        }
    }
    qint64 count = 0;
    do{
        count = reply->read(tmpBuffer, TMP_BUFFER_SIZE);
        if(targetFile!=nullptr && targetFile->isOpen() && count > 0) targetFile->write(tmpBuffer, count);
        buffer->insert(buffer->end(), tmpBuffer, &tmpBuffer[count]);
        if(contentLength != 0){
            uint progressVal = (static_cast<uint>(buffer->size())*100)/ contentLength;
            if(lastProgress!=progressVal){
               lastProgress = progressVal;
               emit progress(TEXT_PROGRESS_DOWNLOADING.arg(progressVal), static_cast<int>(progressVal));
            }
        }
    } while(count > 0);
}

void FirmwareRequest::onFinished(){
    if(targetFile!=nullptr) {
        targetFile->flush();
        targetFile->close();
    }
}

void FirmwareRequest::getResource(QUrl url, QString file, bool addUID, uint length, bool redirect){
    cleanup();
    QNetworkRequest request;
    qDebug() << url;
    if(addUID) {
        QUrlQuery query;
        query.addQueryItem(queryUID, *uid);
        url.setQuery(query.query());
    }
    knwonContentLength = length;
    buffer->clear();
    request.setUrl(url);
    request.setHeader(QNetworkRequest::KnownHeaders::UserAgentHeader, clinetAgent);
    if(redirect){
        request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    }
    targetFileName = file;
    progress(TEXT_PROGRESS_DOWNLOADING.arg(0), static_cast<int>(0));
    reply = manager->get(request);
    connect(reply, SIGNAL(finished()), this, SLOT(onFinished()));
    connect(reply, SIGNAL(readyRead()), this, SLOT(onReadyRead()));
}

void FirmwareRequest::getResourceList(){
    targetFileName = "";
    emit progress(FirmwareRequest::TEXT_CHECKING_4_UPDATES, 10);
    getResource(QUrl(repositoryURL), "", true, 0, true);
    emit progress(FirmwareRequest::TEXT_SENDING_REQUEST, 20);
}
