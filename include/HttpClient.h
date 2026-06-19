#pragma once

#include <QObject>
#include <QQueue>
#include <QMap>
#include <QRecursiveMutex>
#include <QUrl>
#include <QDateTime>
#include <QtQml/qqmlregistration.h>

QT_BEGIN_NAMESPACE
class QNetworkAccessManager;
class QNetworkReply;
class QNetworkRequest;
class QIODevice;
QT_END_NAMESPACE

constexpr int REQUEST_TIMEOUT_MS = 20000;
constexpr int MAX_RETRIES = 3;

class HttpClient : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(int activeDownloads READ activeDownloads NOTIFY activeDownloadsChanged)

public:
    explicit HttpClient(int maxConcurrentDownloads = 3, QObject* parent = nullptr);
    ~HttpClient();

    Q_INVOKABLE void addDownload(const QUrl& url, const QString& filePath);
    //void setMaxConcurrentDownloads(int max);

    int activeDownloads() const;

signals:
    void downloadProgress(const QString& filePath, qint64 bytesReceived, qint64 bytesTotal);
    void downloadFinished(const QString& filePath);
    void downloadFailed(const QString& filePath, const QString& errorString);
    void allDownloadsFinished();
    void activeDownloadsChanged();

private slots:
    void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void onReplyFinished();

private:
    struct Download {
        QUrl url;
        QString filePath;
        int retries = 0;
    };

    QNetworkAccessManager* m_networkManager;
    QQueue<Download> m_downloadQueue;
    QMap<QNetworkReply*, Download> m_activeDownloads;

    int m_maxConcurrentDownloads;
    mutable QRecursiveMutex m_mutex;

    bool saveToDisk(const QString& filename, QNetworkReply* reply);
    void processDownloadQueue();
    void startDownload(const Download& download);

    QNetworkRequest createRequest(const QUrl& url) const;
};