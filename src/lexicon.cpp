#include "lexicon.h"
#include "HttpClient.h"
#include "pathing.h"

#include <spdlog/spdlog.h>
#include <QStandardPaths>
#include <QDir>

Lexicon::Lexicon(QObject* parent) : QObject(parent) {
    m_httpClient = new HttpClient(6, this);

    connect(m_httpClient, &HttpClient::downloadFinished, this, [this](const QString& filePath) {
        spdlog::info("Master list updated: {}", filePath.toStdString());
        emit masterListReady(filePath);
        parseMasterList();
    });

    connect(m_httpClient, &HttpClient::downloadFailed, this, [this](const QString& path, const QString& error) {
        spdlog::error("Failed to update master list: {}", error.toStdString());
        emit downloadError(error);
    });
}

Lexicon::~Lexicon() {}


// The master list contains all ESO addons that are hosted on ESOUI
void Lexicon::updateMasterList() { // Served by the MMOUI API
    QUrl url("https://api.mmoui.com/v4/game/ESO/filelist.json");

    QString appData = Pathing::getInstance()->paths().appData;
    QDir().mkpath(appData);
    m_masterListPath = appData + "/filelist.json";

    spdlog::info("Starting master list download to: {}", m_masterListPath.toStdString());
    m_httpClient->addDownload(url, m_masterListPath);
}

void Lexicon::parseMasterList() {

}