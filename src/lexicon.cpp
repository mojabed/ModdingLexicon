#include "lexicon.h"
#include "HttpClient.h"

#include <spdlog/spdlog.h>
#include <QStandardPaths>
#include <QDir>

LexiconQO::LexiconQO(QObject* parent) : QObject(parent) {
    m_httpClient = new HttpClient(1, this);

    connect(m_httpClient, &HttpClient::downloadFinished, this, [this](const QString& filePath) {
        spdlog::info("Master list updated: {}", filePath.toStdString());
        emit masterListReady(filePath);
        });

    connect(m_httpClient, &HttpClient::downloadFailed, this, [this](const QString& path, const QString& error) {
        spdlog::error("Failed to update master list: {}", error.toStdString());
        emit downloadError(error);
        });
}

LexiconQO::~LexiconQO() {}

void LexiconQO::updateMasterList() {
    QUrl url("https://api.mmoui.com/v4/game/ESO/filelist.json");

    QString appData = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(appData);
    QString targetPath = appData + "/filelist.json";

    spdlog::info("Starting master list download to: {}", targetPath.toStdString());
    m_httpClient->addDownload(url, targetPath);
}