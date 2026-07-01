#include "HttpClient.h"

#include <spdlog/spdlog.h>
#include <QSaveFile>
#include <QNetworkAccessManager>
#include <QNetworkReply>

HttpClient::HttpClient(int maxConcurrentDownloads, QObject* parent)
    : QObject(parent), m_maxConcurrentDownloads(maxConcurrentDownloads) {
    m_networkManager = new QNetworkAccessManager(this);
    m_networkManager->setTransferTimeout(REQUEST_TIMEOUT_MS);
}

HttpClient::~HttpClient() {
    if (m_networkManager) {
        m_networkManager->clearConnectionCache();
        m_networkManager->clearAccessCache();
    }
}

void HttpClient::addDownload(const QUrl& url, const QString& filePath) {
    if (!url.isValid()) {
        spdlog::error("Invalid URL: {}", url.toString().toStdString());
        emit downloadFailed(filePath, "Invalid URL");
        return;
    }
    if (filePath.isEmpty()) {
        spdlog::error("Empty file path provided for URL: {}", url.toString().toStdString());
        emit downloadFailed(filePath, "Empty file path");
        return;
    }

    {
        QMutexLocker locker(&m_mutex);
        m_downloadQueue.enqueue({ url, filePath, 0 });
    }
    processDownloadQueue();
}

void HttpClient::processDownloadQueue() {
    while (true) {
        Download download;
        bool found = false;
        {
            QMutexLocker locker(&m_mutex);
            if (m_activeDownloads.size() < m_maxConcurrentDownloads && !m_downloadQueue.isEmpty()) {
                download = m_downloadQueue.dequeue();
                found = true;
            }
        }
        
        if (found) {
            startDownload(download);
        } else {
            break;
        }
    }
}

void HttpClient::startDownload(const Download& download) {
    spdlog::info("HttpClient: Starting download of {} to {}", 
                 download.url.toString().toStdString(), 
                 download.filePath.toStdString());
                 
    QNetworkRequest request = createRequest(download.url);
    QNetworkReply* reply = m_networkManager->get(request);

    {
        QMutexLocker locker(&m_mutex);
        m_activeDownloads[reply] = download;
    }
    emit activeDownloadsChanged();

    connect(reply, &QNetworkReply::finished, this, &HttpClient::onReplyFinished);
    connect(reply, &QNetworkReply::downloadProgress, this, &HttpClient::onDownloadProgress);
}

void HttpClient::onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal) {
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;

    QString filePath;
    {
        QMutexLocker locker(&m_mutex);
        if (m_activeDownloads.contains(reply)) {
            filePath = m_activeDownloads[reply].filePath;
        }
    }

    if (!filePath.isEmpty()) {
        emit downloadProgress(filePath, bytesReceived, bytesTotal);
    }
}

void HttpClient::onReplyFinished() {
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;

    Download download;
    {
        QMutexLocker locker(&m_mutex);
        download = m_activeDownloads.take(reply);
    }
    emit activeDownloadsChanged();

    if (reply->error() == QNetworkReply::NoError) {
        if (saveToDisk(download.filePath, reply)) {
            emit downloadFinished(download.filePath);
        }
    } else {
        if (download.retries < MAX_RETRIES) {
            download.retries++;
            spdlog::warn("Retrying {} (Attempt {})", download.url.toString().toStdString(), download.retries);
            QMutexLocker locker(&m_mutex);
            m_downloadQueue.enqueue(download);
        } else {
            spdlog::error("Download failed: {}", reply->errorString().toStdString());
            emit downloadFailed(download.filePath, reply->errorString());
        }
    }

    reply->deleteLater();
    processDownloadQueue();

    if (activeDownloads() == 0) {
        m_networkManager->clearConnectionCache();
    }
}

bool HttpClient::saveToDisk(const QString& filename, QNetworkReply* reply) {
    QSaveFile file(filename);
    if (!file.open(QIODevice::WriteOnly)) {
        spdlog::error("Cannot open file for writing: {}", filename.toStdString());
        return false;
    }

    file.write(reply->readAll());
    if (!file.commit()) {
        spdlog::error("Failed to commit file: {}", filename.toStdString());
        return false;
    }
    return true;
}

QNetworkRequest HttpClient::createRequest(const QUrl& url) const {
    QNetworkRequest request(url);
    request.setRawHeader("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:137.0) Gecko/20100101 Firefox/137.0");
    request.setRawHeader("Accept", "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8");
    request.setRawHeader("Accept-Language", "en-US,en;q=0.5");
    request.setRawHeader("Accept-Encoding", "identity");
    request.setRawHeader("Connection", "close");
    request.setRawHeader("Upgrade-Insecure-Requests", "1");
    request.setRawHeader("Sec-Fetch-Dest", "document");
    request.setRawHeader("Sec-Fetch-Mode", "navigate");
    request.setRawHeader("Sec-Fetch-Site", "none");
    request.setRawHeader("Sec-Fetch-User", "?1");

    request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysNetwork);
    request.setAttribute(QNetworkRequest::CacheSaveControlAttribute, false);

    return request;
}

int HttpClient::activeDownloads() const {
    QMutexLocker locker(&m_mutex);
    return m_activeDownloads.size();
}