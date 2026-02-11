#include <spdlog/spdlog.h>
#include <QStandardPaths>
#include <QDir>

#include "lexicon.h"
#include "HttpClient.h"
#include "pathing.h"
#include "parser.h"

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

    updateMasterList();
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
    spdlog::info("Parsing master list from: {}", m_masterListPath.toStdString());
    QFile file(m_masterListPath);
    if (!file.open(QIODevice::ReadOnly)) {
        spdlog::error("Failed to open master list file for parsing: {}", m_masterListPath.toStdString());
        return;
    }
    QByteArray jsonData = file.readAll();
    file.close();

    m_mods = Parser::parseEsoMods(jsonData);

    // Test output
    /*for (const auto& mod : m_mods) {
        spdlog::info("Mod: {} by {} (Version: {})", mod.title.toStdString(), mod.author.toStdString(), mod.version.toStdString());

        for (const auto& addon : mod.addons) {
            spdlog::info(" Addon: {} (version: {})", addon.path.toStdString(), addon.addOnVersion.toStdString());
        }
    }*/
}