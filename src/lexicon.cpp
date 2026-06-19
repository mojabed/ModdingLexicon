#include <spdlog/spdlog.h>
#include <QStandardPaths>
#include <QDir>
#include <QFileInfo>

#include "lexicon.h"
#include "HttpClient.h"
#include "pathing.h"
#include "parser.h"
#include "AddonModel.h"
#include "AddonFilterModel.h"

Lexicon::Lexicon(QObject* parent) : QObject(parent) {
    m_httpClient = new HttpClient(6, this);
    m_addonModel = new AddonModel(this);
    m_installedAddonsFilter = new AddonFilterModel(this);

    m_installedAddonsFilter->setSourceModel(m_addonModel);
    m_installedAddonsFilter->setShowInstalledOnly(true);

    connect(m_httpClient, &HttpClient::downloadFinished, this, [this](const QString& filePath) {
        spdlog::info("Master list updated: {}", filePath.toStdString());
        emit masterListReady(filePath);
        parseMasterList();
    });

    connect(m_httpClient, &HttpClient::downloadFailed, this, [this](const QString& path, const QString& error) {
        spdlog::error("Failed to update master list: {}", error.toStdString());
        emit downloadError(error);
    });

    QString appData = Pathing::getInstance()->paths().appData;
    QDir().mkpath(appData);
    m_masterListPath = appData + "/filelist.json";

    updateMasterList();
}

Lexicon::~Lexicon() {}

AddonModel* Lexicon::addonModel() const {
    return m_addonModel;
}

AddonFilterModel* Lexicon::installedAddonsFilter() const {
    return m_installedAddonsFilter;
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

    checkInstalledAddons();
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
    checkInstalledAddons();
    m_addonModel->setMods(m_mods);

    spdlog::info("Loaded {} mods into model", m_mods.count());
}

void Lexicon::checkInstalledAddons() {
    QString addonsPath = Pathing::getInstance()->paths().addons;

    if (addonsPath.isEmpty()) {
        spdlog::warn("Addons path is empty while checking for installed addons.");
        return;
    }

    QDir addonsDir(addonsPath);
    if (!addonsDir.exists()) {
        spdlog::warn("Addons directory does not exist: {}", addonsPath.toStdString());
        return;
    }

    QStringList installedDirs = addonsDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);

    for (ModInfo& mod : m_mods) {
        if (installedDirs.contains(mod.title, Qt::CaseInsensitive)) {
            mod.isInstalled = true;
            mod.installPath = addonsDir.filePath(mod.title);
            //spdlog::info("Found installed addon: {} at {}", mod.title.toStdString(), mod.installPath.toStdString());
        }
    }
}