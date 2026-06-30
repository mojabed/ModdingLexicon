#include <spdlog/spdlog.h>
#include <QStandardPaths>
#include <QDir>
#include <QProcess>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QSet>
#include <chrono>
#include <QtConcurrent>
#include <algorithm>

#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>

#include "lexicon.h"
#include "HttpClient.h"
#include "pathing.h"
#include "parser.h"
#include "AddonModel.h"
#include "AddonFilterModel.h"
#include "CategoryModel.h"
#include "descriptionparser.h"
#include "descriptionfetcher.h"

Lexicon::Lexicon(QObject* parent) : QObject(parent) {
    m_httpClient = new HttpClient(3, this);
    m_addonModel = new AddonModel(this);
    m_installedAddonsFilter = new AddonFilterModel(this);
    m_categoryModel = new CategoryModel(this);
    m_descriptionFetcher = new DescriptionFetcher(this);

    m_installedAddonsFilter->setSourceModel(m_addonModel);
    m_installedAddonsFilter->setShowInstalledOnly(true);

    connect(&m_parsingWatcher, &QFutureWatcher<QList<ModInfo>>::finished, this, &Lexicon::onParsingFinished);
    connect(&m_installedCheckWatcher, &QFutureWatcher<void>::finished, this, &Lexicon::onInstalledAddonsCheckFinished);

    connect(m_httpClient, &HttpClient::downloadFinished, this, [this](const QString& filePath) {
        if (filePath == m_masterListPath) {
            emit masterListReady(filePath);
            parseMasterList();
        } else if (filePath == m_categoryListPath) {
            parseCategoryList();
        }
    });

    connect(m_httpClient, &HttpClient::downloadFailed, this, [this](const QString& path, const QString& error) {
        spdlog::error("Failed to update master list: {}", error.toStdString());
        emit downloadError(error);
    });

    connect(m_descriptionFetcher, &DescriptionFetcher::descriptionReady,
            this, [this](const QString& url, const QString& desc) {
                m_currentDescription = desc;
                emit currentDescriptionChanged();
                emit addonDescriptionReady(url, desc);
            });

    QString appData = Pathing::getInstance()->paths().appData;
    QDir().mkpath(appData);
    m_masterListPath = appData + "/filelist.json";
    m_categoryListPath = appData + "/categorylist.json";
    m_installedFoldersPath = appData + "/installed_folders.json";

    loadInstalledFolders();

    updateCategoryList();
    updateMasterList();

    // Deferred: prune stale image cache on startup
    QTimer::singleShot(5000, this, [this]() {
        m_descriptionFetcher->pruneImageCache(
            Pathing::getInstance()->paths().appData + "/image_cache");
    });
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

    return true;
}

void Lexicon::updateMasterList() {
    QUrl url("https://api.mmoui.com/v4/game/ESO/filelist.json");
    spdlog::info("Starting master list download to: {}", m_masterListPath.toStdString());
    m_httpClient->addDownload(url, m_masterListPath);
}

void Lexicon::updateCategoryList() {
    QUrl url("https://api.mmoui.com/v4/game/ESO/categorylist.json");
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
    QFile file(m_categoryListPath);
    if (!file.open(QIODevice::ReadOnly)) {
        spdlog::error("Failed to open category list file for parsing: {}", m_categoryListPath.toStdString());
        return;
    }

    QByteArray jsonData = file.readAll();
    file.close();

    m_categoryNames = Parser::parseCategoryNames(jsonData);
    m_categoryIcons = Parser::parseCategoryIcons(jsonData);
    m_addonModel->setCategoryIcons(m_categoryIcons);

    for (auto it = m_categoryNames.begin(); it != m_categoryNames.end(); ++it) {
        if (it.value() == QStringLiteral("Discontinued & Outdated")) {
            m_discontinuedCategoryId = it.key();
            break;
        }
    }

    applyCategoryMetadataToMods();

    if (!m_discontinuedCategoryId.isEmpty()) {
        m_mods.erase(std::remove_if(m_mods.begin(), m_mods.end(),
            [this](const ModInfo& mod) { return mod.categoryId == m_discontinuedCategoryId; }),
            m_mods.end());
    }

    if (!m_mods.isEmpty()) {
        populateCategories();
    }
}

void Lexicon::onParsingFinished() {
    m_mods = m_parsingWatcher.result();

    if (m_mods.isEmpty()) {
        spdlog::error("Failed to parse master list or list is empty");
        return;
    }

    applyCategoryMetadataToMods();

    if (!m_discontinuedCategoryId.isEmpty()) {
        m_mods.erase(std::remove_if(m_mods.begin(), m_mods.end(),
            [this](const ModInfo& mod) { return mod.categoryId == m_discontinuedCategoryId; }),
            m_mods.end());
    }

    checkInstalledAddons();

    m_addonModel->setMods(m_mods);
    m_addonModel->setCategoryIcons(m_categoryIcons);
    spdlog::info("Loaded {} mods into model", m_mods.count());

    populateCategories();
}

void Lexicon::checkInstalledAddons() {
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

        spdlog::info("Found {} installed addon directories", installedDirs.size());

        QSet<QString> installedDirsSet;
        installedDirsSet.reserve(installedDirs.size());
        for (const QString& dir : installedDirs) {
            installedDirsSet.insert(dir.toLower());
        }

        int matchedCount = 0;
        for (ModInfo& mod : m_mods) {
            mod.isInstalled = false;
            mod.installPath.clear();

            const QString lowerTitle = mod.title.toLower();

            if (installedDirsSet.contains(lowerTitle)) {
                mod.isInstalled = true;
                mod.installPath = addonsDir.filePath(mod.title);
                matchedCount++;
                continue;
            }

            QString normalizedTitle = lowerTitle;
            normalizedTitle.remove(' ');
            if (normalizedTitle != lowerTitle) {
                for (const QString& dir : installedDirs) {
                    QString normalizedDir = dir.toLower();
                    normalizedDir.remove(' ');
                    if (normalizedTitle == normalizedDir) {
                        mod.isInstalled = true;
                        mod.installPath = addonsDir.filePath(dir);
                        matchedCount++;
                        break;
                    }
                }
            }

            if (!mod.isInstalled && m_installedFolders.contains(mod.id)) {
                const QStringList tracked = m_installedFolders[mod.id];
                for (const QString& folder : tracked) {
                    if (installedDirsSet.contains(folder.toLower())) {
                        mod.isInstalled = true;
                        mod.installPath = addonsDir.filePath(folder);
                        matchedCount++;
                        break;
                    }
                }
            }
        }

        spdlog::info("Installed check: {} addons matched", matchedCount);

        });

    m_installedCheckWatcher.setFuture(checkFuture);
}

void Lexicon::onInstalledAddonsCheckFinished() {
    m_addonModel->setMods(m_mods);
}

void Lexicon::applyCategoryMetadataToMods() {
    for (ModInfo& mod : m_mods) {
        if (!mod.categoryId.isEmpty()) {
            if (m_categoryNames.contains(mod.categoryId)) {
                mod.categoryName = m_categoryNames.value(mod.categoryId);
            }
        }
    }
}

void Lexicon::populateCategories() {
    if (!m_categoryModel) {
        spdlog::error("m_categoryModel is null");
        return;
    }

    if (m_mods.isEmpty()) {
        spdlog::warn("m_mods is empty, nothing to categorize");
        return;
    }

    try {
        QMap<QString, int> categoryMap;
        QMap<QString, QString> categoryNames;

        for (int i = 0; i < m_mods.count(); ++i) {
            const ModInfo& mod = m_mods.at(i);
            const QString categoryId = mod.categoryId;
            categoryMap[categoryId]++;
            if (!mod.categoryName.isEmpty()) {
                categoryNames[categoryId] = mod.categoryName;
            }
        }

        QList<CategoryInfo> categories;
        for (auto it = categoryMap.begin(); it != categoryMap.end(); ++it) {
            const QString displayName = categoryNames.value(it.key());
            const QString finalName = displayName.isEmpty()
                ? (it.key().isEmpty() ? "Uncategorized" : it.key())
                : displayName;

            // Skip Discontinued & Outdated category
            if (finalName == QStringLiteral("Discontinued & Outdated")) {
                continue;
            }

            CategoryInfo info;
            info.categoryId = it.key();
            info.categoryName = finalName;
            info.iconSource = m_categoryIcons.contains(it.key())
                ? m_categoryIcons.value(it.key())
                : iconForCategoryId(it.key());
            info.addonCount = it.value();
            categories.append(info);
        }

        m_categoryModel->setCategories(categories);
    } catch (const std::exception& e) {
        spdlog::error("Exception in populateCategories: {}", e.what());
    }
}

void Lexicon::installAddon(const QString& modId, const QString& title, const QString& downloadUrl)
{
    if (downloadUrl.isEmpty()) {
        spdlog::error("installAddon: empty download URL for mod {}", modId.toStdString());
        emit addonInstallFailed(modId, "No download URL available");
        return;
    }

    const QString addonsPath = Pathing::getInstance()->paths().addons;
    if (addonsPath.isEmpty()) {
        spdlog::error("installAddon: Addons path is empty");
        emit addonInstallFailed(modId, "Addons directory not configured");
        return;
    }

    QDir().mkpath(addonsPath);

    const QString tempFilePath = QDir::tempPath() + QStringLiteral("/moddinglexicon_%1.zip").arg(modId);

    spdlog::info("installAddon: downloading {} to {}", downloadUrl.toStdString(), tempFilePath.toStdString());
    emit addonInstallStarted(modId);

    auto* manager = new QNetworkAccessManager(this);
    QUrl url(downloadUrl);
    QNetworkRequest request(url);
    request.setRawHeader("User-Agent", "ModdingLexicon/1.0");
    request.setRawHeader("Accept", "*/*");
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysNetwork);

    QNetworkReply* reply = manager->get(request);

    connect(reply, &QNetworkReply::downloadProgress, this, [this, modId](qint64 received, qint64 total) {
        if (total > 0) {
            int percent = static_cast<int>((received * 100) / total);
            emit addonInstallProgress(modId, percent);
        }
    });

    connect(reply, &QNetworkReply::finished, this, [this, reply, manager, modId, addonsPath, tempFilePath]() {
        reply->deleteLater();
        manager->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            spdlog::error("installAddon: download failed: {}", reply->errorString().toStdString());
            QFile::remove(tempFilePath);
            emit addonInstallFailed(modId, reply->errorString());
            return;
        }

        QFile file(tempFilePath);
        if (!file.open(QIODevice::WriteOnly)) {
            spdlog::error("installAddon: failed to write temp file {}", tempFilePath.toStdString());
            emit addonInstallFailed(modId, "Failed to save downloaded file");
            return;
        }
        file.write(reply->readAll());
        file.close();

        spdlog::info("installAddon: downloaded zip to {}, extracting...", tempFilePath.toStdString());

        // Snapshot AddOns directory before extraction
        QDir addonsDir(addonsPath);
        const QStringList beforeDirs = addonsDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);

        // Extract using powershell
        auto* process = new QProcess(this);
        process->setWorkingDirectory(addonsPath);

        connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                this, [this, process, modId, addonsPath, tempFilePath, beforeDirs](int exitCode, QProcess::ExitStatus) {
                    process->deleteLater();

                    if (exitCode != 0) {
                        QString err = process->readAllStandardError();
                        spdlog::error("installAddon: extraction failed: {}", err.toStdString());
                        QFile::remove(tempFilePath);
                        emit addonInstallFailed(modId, "Extraction failed: " + err);
                        return;
                    }

                    spdlog::info("installAddon: extraction complete for {}", modId.toStdString());

                    // Find newly created folders and track them
                    QDir dir(addonsPath);
                    const QStringList afterDirs = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
                    trackInstalledFolders(modId, beforeDirs, afterDirs);

                    QFile::remove(tempFilePath);
                    refreshInstalledStatus();
                    emit addonInstallFinished(modId);
                });

        QString command = QStringLiteral(
            "powershell -NoProfile -Command \"Expand-Archive -LiteralPath '%1' -DestinationPath '%2' -Force\"")
            .arg(tempFilePath, addonsPath);

        process->start(command);
    });
}

void Lexicon::uninstallAddon(const QString& modId, const QString& title)
{
    Q_UNUSED(title)

    const QString addonsPath = Pathing::getInstance()->paths().addons;
    if (addonsPath.isEmpty()) {
        spdlog::error("uninstallAddon: Addons path is empty");
        return;
    }

    if (m_installedFolders.contains(modId)) {
        const QStringList folders = m_installedFolders.take(modId);
        saveInstalledFolders();

        QDir addonsDir(addonsPath);
        for (const QString& folder : folders) {
            const QString fullPath = addonsDir.filePath(folder);
            spdlog::info("uninstallAddon: removing {}", fullPath.toStdString());
            QDir(fullPath).removeRecursively();
        }
        emit addonUninstallFinished(modId);
        refreshInstalledStatus();
        return;
    }

    QDir addonsDir(addonsPath);
    if (!addonsDir.exists()) {
        spdlog::warn("uninstallAddon: Addons directory does not exist");
        return;
    }

    const QString lowerTitle = title.toLower();
    const QString lowerTitleNoSpaces = QString(lowerTitle).remove(' ');

    const QStringList dirs = addonsDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString& dir : dirs) {
        QString lowerDir = dir.toLower();
        if (lowerDir == lowerTitle || lowerDir == lowerTitleNoSpaces) {
            const QString fullPath = addonsDir.filePath(dir);
            spdlog::info("uninstallAddon: removing {} (fallback)", fullPath.toStdString());
            QDir(fullPath).removeRecursively();
            emit addonUninstallFinished(modId);
            refreshInstalledStatus();
            return;
        }
        QString lowerDirNoSpaces = lowerDir;
        lowerDirNoSpaces.remove(' ');
        if (lowerDirNoSpaces == lowerTitleNoSpaces) {
            const QString fullPath = addonsDir.filePath(dir);
            spdlog::info("uninstallAddon: removing {} (fallback)", fullPath.toStdString());
            QDir(fullPath).removeRecursively();
            emit addonUninstallFinished(modId);
            refreshInstalledStatus();
            return;
        }
    }

    spdlog::warn("uninstallAddon: no folders found for mod {}", modId.toStdString());
}

void Lexicon::refreshInstalledStatus()
{
    checkInstalledAddons();
}

void Lexicon::loadInstalledFolders()
{
    QFile file(m_installedFoldersPath);
    if (!file.open(QIODevice::ReadOnly))
        return;

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (!doc.isObject())
        return;

    const QJsonObject root = doc.object();
    for (auto it = root.begin(); it != root.end(); ++it) {
        const QJsonArray arr = it.value().toArray();
        QStringList folders;
        folders.reserve(arr.size());
        for (const QJsonValue& v : arr)
            folders.append(v.toString());
        if (!folders.isEmpty())
            m_installedFolders[it.key()] = folders;
    }

    spdlog::info("Loaded {} installed folder entries", m_installedFolders.size());
}

void Lexicon::saveInstalledFolders()
{
    QJsonObject root;
    for (auto it = m_installedFolders.begin(); it != m_installedFolders.end(); ++it) {
        QJsonArray arr;
        for (const QString& folder : it.value())
            arr.append(folder);
        root[it.key()] = arr;
    }

    QFile file(m_installedFoldersPath);
    if (!file.open(QIODevice::WriteOnly)) {
        spdlog::error("Failed to save installed folders: {}", m_installedFoldersPath.toStdString());
        return;
    }
    file.write(QJsonDocument(root).toJson());
    file.close();
}

void Lexicon::trackInstalledFolders(const QString& modId, const QStringList& before, const QStringList& after)
{
    QSet<QString> beforeSet(before.begin(), before.end());
    QStringList newFolders;
    for (const QString& dir : after) {
        if (!beforeSet.contains(dir))
            newFolders.append(dir);
    }

    if (newFolders.isEmpty()) {
        spdlog::warn("trackInstalledFolders: no new folders detected for mod {}", modId.toStdString());
        return;
    }

    m_installedFolders[modId] = newFolders;
    saveInstalledFolders();
    spdlog::info("Tracked {} folders for mod {}: {}", newFolders.size(), modId.toStdString(),
                 newFolders.join(", ").toStdString());
}

void Lexicon::fetchAddonDescription(const QString& fileInfoUrl)
{
    if (m_descriptionFetcher)
        m_descriptionFetcher->fetch(fileInfoUrl);
}
