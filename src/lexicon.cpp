#include <spdlog/spdlog.h>
#include <QStandardPaths>
#include <QDir>
#include <QFileInfo>
#include <chrono>
#include <QtConcurrent>
#include <QMap>
#include <QFile>

#include "lexicon.h"
#include "HttpClient.h"
#include "pathing.h"
#include "parser.h"
#include "AddonModel.h"
#include "AddonFilterModel.h"
#include "CategoryModel.h"

Lexicon::Lexicon(QObject* parent) : QObject(parent) {
    m_httpClient = new HttpClient(3, this);
    m_addonModel = new AddonModel(this);
    m_installedAddonsFilter = new AddonFilterModel(this);
    m_categoryModel = new CategoryModel(this);

    m_installedAddonsFilter->setSourceModel(m_addonModel);
    m_installedAddonsFilter->setShowInstalledOnly(true);

    connect(&m_parsingWatcher, &QFutureWatcher<QList<ModInfo>>::finished, this, &Lexicon::onParsingFinished);
    connect(&m_installedCheckWatcher, &QFutureWatcher<void>::finished, this, &Lexicon::onInstalledAddonsCheckFinished);

    connect(m_httpClient, &HttpClient::downloadFinished, this, [this](const QString& filePath) {
        if (filePath == m_masterListPath) {
            spdlog::info("Master list updated: {}", filePath.toStdString());
            emit masterListReady(filePath);
            parseMasterList();
        } else if (filePath == m_categoryListPath) {
            spdlog::info("Category list updated: {}", filePath.toStdString());
            parseCategoryList();
        }
    });

    connect(m_httpClient, &HttpClient::downloadFailed, this, [this](const QString& path, const QString& error) {
        spdlog::error("Failed to update master list: {}", error.toStdString());
        emit downloadError(error);
    });

    QString appData = Pathing::getInstance()->paths().appData;
    QDir().mkpath(appData);
    m_masterListPath = appData + "/filelist.json";
    m_categoryListPath = appData + "/categorylist.json";

    updateCategoryList();
    updateMasterList();
}

Lexicon::~Lexicon() {}

AddonModel* Lexicon::addonModel() const {
    return m_addonModel;
}

AddonFilterModel* Lexicon::installedAddonsFilter() const {
    return m_installedAddonsFilter;
}

CategoryModel* Lexicon::categoryModel() const {
    return m_categoryModel;
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

void Lexicon::updateMasterList() {
    QUrl url("https://api.mmoui.com/v4/game/ESO/filelist.json");
    spdlog::info("Starting master list download to: {}", m_masterListPath.toStdString());
    m_httpClient->addDownload(url, m_masterListPath);
}

void Lexicon::updateCategoryList() {
    QUrl url("https://api.mmoui.com/v4/game/ESO/categorylist.json");
    spdlog::info("Starting category list download to: {}", m_categoryListPath.toStdString());
    m_httpClient->addDownload(url, m_categoryListPath);
}

void Lexicon::parseMasterList() {
    spdlog::info("Parsing master list from: {}", m_masterListPath.toStdString());

    auto parseFuture = QtConcurrent::run([this]() -> QList<ModInfo> {
        QFile file(m_masterListPath);
        if (!file.open(QIODevice::ReadOnly)) {
            spdlog::error("Failed to open master list file for parsing: {}", m_masterListPath.toStdString());
            return QList<ModInfo>();
        }
        QByteArray jsonData = file.readAll();
        file.close();

        return Parser::parseEsoMods(jsonData);
        });

    m_parsingWatcher.setFuture(parseFuture);
}

void Lexicon::parseCategoryList() {
    spdlog::info("Parsing category list from: {}", m_categoryListPath.toStdString());

    QFile file(m_categoryListPath);
    if (!file.open(QIODevice::ReadOnly)) {
        spdlog::error("Failed to open category list file for parsing: {}", m_categoryListPath.toStdString());
        return;
    }

    QByteArray jsonData = file.readAll();
    file.close();

    m_categoryNames = Parser::parseCategoryNames(jsonData);
    applyCategoryNamesToMods();

    if (!m_mods.isEmpty()) {
        populateCategories();
    }
}

void Lexicon::onParsingFinished() {
    auto startTime = std::chrono::high_resolution_clock::now();

    m_mods = m_parsingWatcher.result();

    if (m_mods.isEmpty()) {
        spdlog::error("Failed to parse master list or list is empty");
        return;
    }

    applyCategoryNamesToMods();
    checkInstalledAddons();

    m_addonModel->setMods(m_mods);
    spdlog::info("Loaded {} mods into model", m_mods.count());

    populateCategories();

    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    spdlog::info("parseMasterList took {}ms", duration.count());
}

void Lexicon::checkInstalledAddons() {
    auto startTime = std::chrono::high_resolution_clock::now();

    auto checkFuture = QtConcurrent::run([this]() {
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

        QSet<QString> installedDirsSet;
        installedDirsSet.reserve(installedDirs.size());
        for (const QString& dir : installedDirs) {
            installedDirsSet.insert(dir.toLower());
        }

        for (ModInfo& mod : m_mods) {
            if (installedDirsSet.contains(mod.title.toLower())) {
                mod.isInstalled = true;
                mod.installPath = addonsDir.filePath(mod.title);
            }
        }
        });

    m_installedCheckWatcher.setFuture(checkFuture);
}

void Lexicon::onInstalledAddonsCheckFinished() {
    auto startTime = std::chrono::high_resolution_clock::now();

    m_addonModel->setMods(m_mods);

    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    spdlog::info("checkInstalledAddons took {}ms", duration.count());
}

void Lexicon::applyCategoryNamesToMods() {
    for (ModInfo& mod : m_mods) {
        if (!mod.categoryId.isEmpty() && m_categoryNames.contains(mod.categoryId)) {
            mod.categoryName = m_categoryNames.value(mod.categoryId);
        }
    }
}

void Lexicon::populateCategories() {
    spdlog::info("populateCategories() called");

    if (!m_categoryModel) {
        spdlog::error("m_categoryModel is null!");
        return;
    }

    spdlog::info("m_mods count: {}", m_mods.count());

    if (m_mods.isEmpty()) {
        spdlog::warn("m_mods is empty, nothing to categorize");
        return;
    }

    try {
        QMap<QString, int> categoryMap;
        QMap<QString, QString> categoryNames;

        spdlog::info("Starting to iterate through mods");
        for (int i = 0; i < m_mods.count(); ++i) {
            const ModInfo& mod = m_mods.at(i);
            const QString categoryId = mod.categoryId;
            categoryMap[categoryId]++;
            if (!mod.categoryName.isEmpty()) {
                categoryNames[categoryId] = mod.categoryName;
            }
        }

        spdlog::info("Found {} unique categories", categoryMap.count());

        QList<CategoryInfo> categories;
        for (auto it = categoryMap.begin(); it != categoryMap.end(); ++it) {
            CategoryInfo info;
            info.categoryId = it.key();
            const QString displayName = categoryNames.value(it.key());
            info.categoryName = displayName.isEmpty()
                ? (it.key().isEmpty() ? "Uncategorized" : it.key())
                : displayName;
            info.iconSource = iconForCategoryId(it.key());
            info.addonCount = it.value();
            categories.append(info);
        }

        spdlog::info("Created {} category objects, calling setCategories", categories.count());
        m_categoryModel->setCategories(categories);
        spdlog::info("Populated {} categories", categories.count());
    } catch (const std::exception& e) {
        spdlog::error("Exception in populateCategories: {}", e.what());
    }
}