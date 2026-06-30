#include <spdlog/spdlog.h>
#include <QStandardPaths>
#include <QDir>
#include <QProcess>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QSet>
#include <QTimer>
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
    m_installedAddonsFilter->setExcludeBelowApiVersion(0);

    connect(&m_parsingWatcher, &QFutureWatcher<QList<ModInfo>>::finished, this, &Lexicon::onParsingFinished);
    connect(&m_installedCheckWatcher, &QFutureWatcher<void>::finished, this, &Lexicon::onInstalledAddonsCheckFinished);

    connect(m_httpClient, &HttpClient::downloadFinished, this, [this](const QString& filePath) {
        if (filePath == m_masterListPath) {
            emit masterListReady(filePath);
            parseMasterList();
        } else if (filePath == m_categoryListPath) {
            parseCategoryList();
        } else if (filePath == m_gameConfigPath) {
            parseGameConfig();
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
    m_gameConfigPath = appData + "/gameconfig.json";
    m_installedFoldersPath = appData + "/installed_folders.json";

    // Defaults in case gameconfig.json hasn't loaded yet
    m_gameVersion = "12.0.0";
    m_gameVersionName = "Season Zero Pt.2";

    loadInstalledFolders();

    // Try loading cached gameconfig first, then update
    parseGameConfig();
    updateGameConfig();
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
    m_installedAddonsFilter->refreshFilter();

    return true;
}

void Lexicon::updateGameConfig() {
    QUrl url("https://api.mmoui.com/v4/game/ESO/gameconfig.json");
    m_httpClient->addDownload(url, m_gameConfigPath);
}

void Lexicon::parseGameConfig() {
    QMap<int, QPair<QString, QString>> newMap;

    // Try loading from cached file
    QFile file(m_gameConfigPath);
    if (file.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        file.close();
        if (doc.isObject()) {
            QJsonArray versions = doc.object()["gameVersions"].toArray();
            for (const QJsonValue& val : versions) {
                QJsonObject entry = val.toObject();
                int api = entry["apiVersion"].toInt();
                if (api == 0) continue;
                QString ver = entry["version"].toString();
                QString name = entry["name"].toString().replace('\n', ' ').trimmed();
                newMap[api] = qMakePair(ver, name);
            }
        }
    }

    // Fallback to hardcoded map if file didn't yield usable data
    if (newMap.isEmpty()) {
        if (!m_gameVersionMap.isEmpty()) return; // Already have data, keep it
        spdlog::warn("parseGameConfig: using hardcoded fallback map");
        newMap[101050] = qMakePair(QString("12.0.0"), QString("Season Zero Pt.2"));
        newMap[101049] = qMakePair(QString("11.3.0"), QString("Season Zero"));
        newMap[101048] = qMakePair(QString("11.2.0"), QString("Seasons of the Worm Cult Pt2"));
        newMap[101047] = qMakePair(QString("11.1.0"), QString("Feast of Shadows"));
        newMap[101046] = qMakePair(QString("11.0.0"), QString("Seasons of the Worm Cult Pt1"));
        newMap[101045] = qMakePair(QString("10.3.5"), QString("Fallen Banners"));
        newMap[101044] = qMakePair(QString("10.2.0"), QString("Update 44"));
        newMap[101043] = qMakePair(QString("10.1.0"), QString("Update 43"));
        newMap[101042] = qMakePair(QString("10.0.0"), QString("Gold Road"));
        newMap[101041] = qMakePair(QString("9.3.0"), QString("Scions of Ithelia"));
        newMap[100035] = qMakePair(QString("2.7.0"), QString("Horns of the Reach"));
        newMap[100034] = qMakePair(QString("2.6.0"), QString("Morrowind"));
        newMap[100033] = qMakePair(QString("2.5.0"), QString("Homestead"));
        newMap[100032] = qMakePair(QString("2.4.0"), QString("One Tamriel"));
        newMap[100031] = qMakePair(QString("2.3.0"), QString("Shadows of the Hist"));
        newMap[100030] = qMakePair(QString("2.2.0"), QString("Dark Brotherhood"));
        newMap[100029] = qMakePair(QString("2.1.0"), QString("Thieves Guild"));
        newMap[100028] = qMakePair(QString("1.7.0"), QString("Orsinium"));
        newMap[100027] = qMakePair(QString("1.6.5"), QString("Update 6"));
        newMap[100012] = qMakePair(QString("1.0.0"), QString("Live"));
    }

    if (newMap == m_gameVersionMap) return; // No change

    m_gameVersionMap = newMap;
    auto lastIt = m_gameVersionMap.end(); --lastIt;
    m_gameVersion = lastIt.value().first;
    m_gameVersionName = lastIt.value().second;

    spdlog::info("Loaded {} game versions; latest: {} ({})",
                 m_gameVersionMap.size(), m_gameVersionName.toStdString(), m_gameVersion.toStdString());

    applyGameVersionsToMods();
    if (!m_mods.isEmpty() && m_addonModel) {
        m_addonModel->setMods(m_mods);
        m_installedAddonsFilter->refreshFilter();
    }
    emit gameVersionChanged();
}

void Lexicon::applyGameVersionsToMods() {
    const bool mapReady = !m_gameVersionMap.isEmpty();
    spdlog::info("applyGameVersionsToMods: mapReady={} modCount={} mapSize={}",
                 mapReady, m_mods.size(), m_gameVersionMap.size());

    for (ModInfo& mod : m_mods) {
        if (!mapReady) {
            mod.gameVersionStr = m_gameVersionName + " (" + m_gameVersion + ")";
            continue;
        }

        int apiVersion = mod.maxApiVersion;

        if (apiVersion <= 0) {
            mod.gameVersionStr = m_gameVersionName + " (" + m_gameVersion + ")";
            continue;
        }

        auto it = m_gameVersionMap.lowerBound(apiVersion);
        if (it == m_gameVersionMap.end()) {
            // apiVersion is higher than any known version — use latest
            --it;
        } else if (it.key() > apiVersion) {
            // apiVersion falls between entries — step back to next older version
            if (it == m_gameVersionMap.begin()) {
                // apiVersion is older than oldest known — use oldest
            } else {
                --it;
            }
        }

        const auto& pair = it.value();
        mod.gameVersionStr = pair.second + " (" + pair.first + ")";
    }

    spdlog::info("applyGameVersionsToMods done, sample:");
    for (int i = 0; i < std::min(5, static_cast<int>(m_mods.size())); ++i) {
        const auto& m = m_mods[i];
        spdlog::info("  [{}] id={} maxApi={} gv='{}'", i, m.id.toStdString(), m.maxApiVersion, m.gameVersionStr.toStdString());
    }
}

void Lexicon::refreshCategoryCounts()
{
    if (!m_categoryModel || !m_installedAddonsFilter || !m_addonModel)
        return;

    const int minApi = m_installedAddonsFilter->excludeBelowApiVersion();

    QMap<QString, int> counts;
    const int totalRows = m_addonModel->rowCount();
    for (int i = 0; i < totalRows; ++i) {
        QModelIndex idx = m_addonModel->index(i, 0);

        // Apply apiVersion filter
        if (minApi > 0) {
            int apiVersion = m_addonModel->data(idx, AddonModel::ApiVersionRole).toInt();
            if (apiVersion > 0 && apiVersion < minApi) {
                bool isInstalled = m_addonModel->data(idx, AddonModel::IsInstalledRole).toBool();
                if (!isInstalled)
                    continue;
            }
        }

        QString catId = m_addonModel->data(idx, AddonModel::CategoryIdRole).toString();
        counts[catId]++;
    }
    m_categoryModel->updateCounts(counts);
}

int Lexicon::getAddonApiVersion(const QString& modId) const
{
    for (const ModInfo& mod : m_mods) {
        if (mod.id == modId)
            return mod.maxApiVersion;
    }
    return 0;
}

QString Lexicon::getGameVersionForAddon(const QString& modId) const
{
    for (const ModInfo& mod : m_mods) {
        if (mod.id == modId) {
            spdlog::info("getGameVersionForAddon: id={} gameVersionStr='{}' fallback='{} ({})'",
                         modId.toStdString(), mod.gameVersionStr.toStdString(),
                         m_gameVersionName.toStdString(), m_gameVersion.toStdString());
            if (!mod.gameVersionStr.isEmpty())
                return mod.gameVersionStr;
            break;
        }
    }
    // Fallback: latest game version
    if (!m_gameVersionName.isEmpty())
        return m_gameVersionName + " (" + m_gameVersion + ")";
    return QString();
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

    applyGameVersionsToMods();
    m_addonModel->setMods(m_mods);
    m_installedAddonsFilter->refreshFilter();
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
    m_installedAddonsFilter->refreshFilter();
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

void Lexicon::downloadAndExtractAddon(const QString& modId, const QString& title,
                                     const QString& downloadUrl,
                                     std::function<void(bool, const QString&)> onDone)
{
    const QString addonsPath = Pathing::getInstance()->paths().addons;
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

    connect(reply, &QNetworkReply::finished, this, [this, reply, manager, modId, addonsPath, tempFilePath, onDone]() {
        reply->deleteLater();
        manager->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            spdlog::error("installAddon: download failed: {}", reply->errorString().toStdString());
            QFile::remove(tempFilePath);
            onDone(false, reply->errorString());
            return;
        }

        QFile file(tempFilePath);
        if (!file.open(QIODevice::WriteOnly)) {
            spdlog::error("installAddon: failed to write temp file {}", tempFilePath.toStdString());
            onDone(false, QStringLiteral("Failed to save downloaded file"));
            return;
        }
        file.write(reply->readAll());
        file.close();

        spdlog::info("installAddon: downloaded zip to {}, extracting...", tempFilePath.toStdString());

        QDir addonsDir(addonsPath);
        const QStringList beforeDirs = addonsDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);

        auto* process = new QProcess(this);
        process->setWorkingDirectory(addonsPath);

        connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                this, [this, process, modId, addonsPath, tempFilePath, beforeDirs, onDone](int exitCode, QProcess::ExitStatus) {
                    process->deleteLater();

                    if (exitCode != 0) {
                        QString err = process->readAllStandardError();
                        spdlog::error("installAddon: extraction failed: {}", err.toStdString());
                        QFile::remove(tempFilePath);
                        onDone(false, QStringLiteral("Extraction failed: ") + err);
                        return;
                    }

                    spdlog::info("installAddon: extraction complete for {}", modId.toStdString());

                    QDir dir(addonsPath);
                    const QStringList afterDirs = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
                    trackInstalledFolders(modId, beforeDirs, afterDirs);

                    QFile::remove(tempFilePath);
                    onDone(true, QString());
                });

        QString command = QStringLiteral(
            "powershell -NoProfile -Command \"Expand-Archive -LiteralPath '%1' -DestinationPath '%2' -Force\"")
            .arg(tempFilePath, addonsPath);

        process->start(command);
    });
}

void Lexicon::installDependenciesFor(const QString& modId, const QString& addonsPath)
{
    const ModInfo* mod = nullptr;
    for (const ModInfo& m : m_mods) {
        if (m.id == modId) {
            mod = &m;
            break;
        }
    }
    if (!mod) return;

    // Collect unique dependency titles
    QSet<QString> depTitles;
    for (const Dependencies& dep : mod->addons) {
        for (const QString& title : dep.requiredDependencies)
            depTitles.insert(title);
        for (const QString& title : dep.optionalDependencies)
            depTitles.insert(title);
    }

    if (depTitles.isEmpty()) {
        refreshInstalledStatus();
        emit addonInstallFinished(modId);
        return;
    }

    // Resolve titles to ModInfo entries
    struct DepEntry { QString id; QString title; QString downloadUrl; };
    QList<DepEntry> toInstall;
    for (const QString& depTitle : depTitles) {
        const QString lowerTitle = depTitle.toLower();
        for (const ModInfo& m : m_mods) {
            if (m.title.toLower() == lowerTitle && !m.isInstalled && m.id != modId) {
                toInstall.append({ m.id, m.title, m.downloadUrl.toString() });
                break;
            }
        }
    }

    if (toInstall.isEmpty()) {
        refreshInstalledStatus();
        emit addonInstallFinished(modId);
        return;
    }

    emit addonInstallStatusChanged(modId, QStringLiteral("installing_dependencies"));

    // Install sequentially using index based recursion
    auto sharedIdx = std::make_shared<int>(0);
    auto sharedList = std::make_shared<QList<DepEntry>>(toInstall);

    std::function<void()> installNext;
    installNext = [this, modId, sharedIdx, sharedList, &installNext]() {
        if (*sharedIdx >= sharedList->size()) {
            refreshInstalledStatus();
            emit addonInstallFinished(modId);
            return;
        }

        const DepEntry& dep = sharedList->at(*sharedIdx);
        emit addonDependencyInstallStarted(modId, dep.title);

        downloadAndExtractAddon(dep.id, dep.title, dep.downloadUrl,
            [this, modId, sharedIdx, &installNext](bool success, const QString& error) {
                if (!success) {
                    spdlog::warn("installAddon: dependency failed: {}", error.toStdString());
                    // Continue with next dependency even if one fails
                }
                (*sharedIdx)++;
                installNext();
            });
    };

    installNext();
}

void Lexicon::installAddon(const QString& modId, const QString& title, const QString& downloadUrl)
{
    Q_UNUSED(title)

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

    downloadAndExtractAddon(modId, QString() /*title unused*/, downloadUrl,
        [this, modId, addonsPath](bool success, const QString& error) {
            if (!success) {
                emit addonInstallFailed(modId, error);
                return;
            }
            installDependenciesFor(modId, addonsPath);
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
