#ifndef REMOTEFILEINFO_H
#define REMOTEFILEINFO_H

#include <QJsonValue>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QUrl>
#include <vector>
class RemoteFileInfo
{
public:
    enum ResourceType {
        Firmware,
        Archive
    };

    RemoteFileInfo();
    void parse(QJsonValue value);
    bool isValid(char* data, int length);
    static bool ParseResourceList(char* data, int length, std::vector<RemoteFileInfo>& results, QString& error);
    ResourceType type;
    QString name;
    QString fileName;
    QString version;
    QString md5Sum;
    int size;
    int startAddress;
    QUrl url;

    const QString JSON_type = "type";
    const QString JSON_name = "name";
    const QString JSON_fileName = "fileName";
    const QString JSON_version = "version";
    const QString JSON_md5Sum = "md5Sum";
    const QString JSON_size = "size";
    const QString JSON_startAddress = "startAddress";
    const QString JSON_url = "url";

    static const QString TEXT_invalidResponse;
};

#endif // REMOTEFILEINFO_H
