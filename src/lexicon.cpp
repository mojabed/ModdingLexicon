#include <spdlog/spdlog.h>
#include <QStandardPaths>
#include <QDir>
#include <QFileInfo>

#include "lexicon.h"
#include "HttpClient.h"
#include "pathing.h"
#include "parser.h"
#include "AddonModel.h"

Lexicon::Lexicon(QObject* parent) : QObject(parent) {
    m_httpClient = new HttpClient(6, this);
    m_addonModel = new AddonModel(this);

    connect(m_httpClient, &HttpClient::downloadFinished, this, [this](const QString& filePath) {
        spdlog::info("Master list updated: {}", filePath.toStdString());
        emit masterListReady(filePath);
        parseMasterList();
    });

    connect(m_httpClient, &HttpClient::downloadFailed, this, [this](const QString& path, const QString& error) {
        spdlog::error("Failed to update master list: {}", error.toStdString());
        emit downloadError(error);
    });

    connect(m_httpClient, &HttpClient::remoteFileDateChecked, this,
        [this](const QUrl& url, const QDateTime& lastModified, bool success) {
            Q_UNUSED(url);

            if (!success) {
                spdlog::warn("Failed to check remote file, using cached version if available");
                if (QFileInfo::exists(m_masterListPath) && loadCachedMasterList()) {
                    emit masterListReady(m_masterListPath);
                } else {
                    updateMasterList();
                }
                return;
            }

            QFileInfo localFile(m_masterListPath);

            if (!localFile.exists()) {
                spdlog::info("No local file, downloading...");
                updateMasterList();
                return;
            }

            QDateTime localModified = localFile.lastModified();
            spdlog::info("Local file modified: {}", localModified.toString(Qt::ISODate).toStdString());

            if (lastModified.isValid() && lastModified > localModified) {
                spdlog::info("Remote file is newer, downloading update...");
                updateMasterList();
            } else {
                spdlog::info("Local file is up to date, using cached version");
                if (loadCachedMasterList()) {
                    emit masterListReady(m_masterListPath);
                } else {
                    spdlog::warn("Failed to load cached file, downloading fresh copy");
                    updateMasterList();
                }
            }
        });

    QString appData = Pathing::getInstance()->paths().appData;
    QDir().mkpath(appData);
    m_masterListPath = appData + "/filelist.json";

    checkMasterListUpdate();
}

Lexicon::~Lexicon() {}

AddonModel* Lexicon::addonModel() const {
    return m_addonModel;
}

void Lexicon::checkMasterListUpdate() {
    QUrl url("https://api.mmoui.com/v4/game/ESO/filelist.json");
    m_httpClient->checkRemoteFileDate(url);
}

bool Lexicon::loadCachedMasterList() {
    spdlog::info("Loading cached master list from: {}", m_masterListPath.toStdString());
    QFile file(m_masterListPath);
    if (!file.open(QIODevice::ReadOnly)) {
        spdlog::error("Failed to open cached master list file: {}", m_masterListPath.toStdString());
        return false;
    }
    QByteArray jsonData = file.readAll();
    file.close();

    m_mods = Parser::parseEsoMods(jsonData);
    if (m_mods.isEmpty()) {
        spdlog::error("Failed to parse cached master list or list is empty");
        return false;
    }

    m_addonModel->setMods(m_mods);
    spdlog::info("Loaded {} mods from cache", m_mods.count());
    return true;
}

// The master list contains all ESO addons that are hosted on ESOUI
void Lexicon::updateMasterList() { // Served by the MMOUI API
    QUrl url("https://api.mmoui.com/v4/game/ESO/filelist.json");
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
    m_addonModel->setMods(m_mods);

    spdlog::info("Loaded {} mods into model", m_mods.count());
}