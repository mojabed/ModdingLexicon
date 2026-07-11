#include <spdlog/spdlog.h>
#include <QDir>
#include <QProcess>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QSet>
#include <QTimer>
#include <QRegularExpression>
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
    m_installedAddonsFilter->setSortMode("title");
    m_installedAddonsFilter->setSortOrder(Qt::AscendingOrder);

    connect(&m_parsingWatcher, &QFutureWatcher<QList<ModInfo>>::finished, this, &Lexicon::onParsingFinished);
    connect(&m_installedCheckWatcher, &QFutureWatcher<void>::finished, this, &Lexicon::onInstalledAddonsCheckFinished);

    connect(m_httpClient, &HttpClient::downloadFinished, this, [this](const QString& filePath) {
        if (filePath == m_masterListPath) {
            parseMasterList();
        } else if (filePath == m_categoryListPath) {
            parseCategoryList();
        } else if (filePath == m_gameConfigPath) {
            parseGameConfig();
        }
    });

    connect(m_httpClient, &HttpClient::downloadFailed, this, [this](const QString& path, const QString& error) {
        spdlog::error("Failed to update master list: {}", error.toStdString());
    });

    connect(m_descriptionFetcher, &DescriptionFetcher::descriptionReady,
            this, [this](const QString& url, const QString& desc) {
                m_currentDescription = desc;
                emit currentDescriptionChanged();
            });

    QString appData = Pathing::getInstance()->paths().appData;
    QDir().mkpath(appData);
    m_masterListPath = appData + "/filelist.json";
    m_categoryListPath = appData + "/categorylist.json";
    m_gameConfigPath = appData + "/gameconfig.json";
    m_installedFoldersPath = appData + "/installed_folders.json";

    m_gameVersion = "12.0.0";
    m_gameVersionName = "Season Zero Pt.2";

    loadInstalledFolders();

    parseGameConfig();
    updateGameConfig();
    updateCategoryList();
    updateMasterList();

    QTimer::singleShot(5000, this, [this]() {
        m_descriptionFetcher->pruneImageCache(
            Pathing::getInstance()->paths().appData + "/image_cache");
    });

    // Check for app updates via version.json on launch
    QTimer::singleShot(3000, this, [this]() { checkForAppUpdate(); });
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

void Lexicon::updateGameConfig() {
    QUrl url("https://api.mmoui.com/v4/game/ESO/gameconfig.json");
    m_httpClient->addDownload(url, m_gameConfigPath);
}

void Lexicon::parseGameConfig() {
    QMap<int, QPair<QString, QString>> newMap;

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

    if (newMap.isEmpty()) {
        if (!m_gameVersionMap.isEmpty()) return;
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

    if (newMap == m_gameVersionMap) return;

    m_gameVersionMap = newMap;
    auto lastIt = m_gameVersionMap.end(); --lastIt;
    m_gameVersion = lastIt.value().first;
    m_gameVersionName = lastIt.value().second;

    applyGameVersionsToMods();
    if (!m_mods.isEmpty() && m_addonModel) {
        m_addonModel->setMods(m_mods);
        m_installedAddonsFilter->refreshFilter();
    }
    emit gameVersionChanged();
}

void Lexicon::applyGameVersionsToMods() {
    const bool mapReady = !m_gameVersionMap.isEmpty();

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
            --it;
        } else if (it.key() > apiVersion) {
            if (it != m_gameVersionMap.begin())
                --it;
        }

        const auto& pair = it.value();
        mod.gameVersionStr = pair.second + " (" + pair.first + ")";
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
    int allTotal = 0;
    for (auto it = counts.begin(); it != counts.end(); ++it)
        allTotal += it.value();
    counts[QString()] = allTotal;
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
            if (!mod.gameVersionStr.isEmpty())
                return mod.gameVersionStr;
            break;
        }
    }

    if (!m_gameVersionName.isEmpty())
        return m_gameVersionName + " (" + m_gameVersion + ")";
    return QString();
}

void Lexicon::updateMasterList() {
    QFile::remove(m_masterListPath);
    QUrl url("https://api.mmoui.com/v4/game/ESO/filelist.json");
    m_httpClient->addDownload(url, m_masterListPath);
}

void Lexicon::updateCategoryList() {
    QUrl url("https://api.mmoui.com/v4/game/ESO/categorylist.json");
    m_httpClient->addDownload(url, m_categoryListPath);
}

void Lexicon::parseMasterList() {
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

    populateCategories();
    refreshCategoryCounts();
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

        QStringList installedDirs;
        for (const QString& dir : addonsDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
            // Skip non-addon folders
            if (dir.startsWith('.') || dir.startsWith('_') || dir.endsWith(".zip"))
                continue;
            installedDirs.append(dir);
        }

        QSet<QString> installedDirsSet;
        installedDirsSet.reserve(installedDirs.size());
        for (const QString& dir : installedDirs) {
            QString d = dir.toLower();
            d.remove('\'');
            installedDirsSet.insert(d);
        }

        QMap<QString, int> pathCounts;
        for (const ModInfo& mod : m_mods) {
            if (!mod.addons.isEmpty()) {
                QString pathKey = mod.addons[0].path.toLower();
                pathKey.remove('\'');
                pathCounts[pathKey]++;
            }
        }

        int matchedCount = 0;
        int titleMatch = 0;
        int spaceMatch = 0;
        int trackedMatch = 0;
        int pathMatch = 0;
        for (ModInfo& mod : m_mods) {
            mod.isInstalled = false;
            mod.installPath.clear();

            const QString lowerTitle = mod.title.toLower();
            QString normalizedTitle = lowerTitle;
            normalizedTitle.remove(' ');
            normalizedTitle.remove('\'');

            static const QRegularExpression versionSuffix(QStringLiteral("[\\d.]+$"));
            QString strippedTitle = normalizedTitle;
            strippedTitle.remove(versionSuffix);

            if (installedDirsSet.contains(lowerTitle)) {
                mod.isInstalled = true;
                mod.installPath = addonsDir.filePath(mod.title);
                matchedCount++;
                titleMatch++;
                continue;
            }

            if (normalizedTitle != lowerTitle) {
                for (const QString& dir : installedDirs) {
                    QString normalizedDir = dir.toLower();
                    normalizedDir.remove(' ');
                    normalizedDir.remove('\'');
                    if (normalizedTitle == normalizedDir || strippedTitle == normalizedDir) {
                        mod.isInstalled = true;
                        mod.installPath = addonsDir.filePath(dir);
                        matchedCount++;
                        spaceMatch++;
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
                        trackedMatch++;
                        break;
                    }
                }
            }

                // Match by addons[0].path
                if (!mod.isInstalled && !mod.addons.isEmpty()) {
                    QString pathLower = mod.addons[0].path.toLower();
                    pathLower.remove('\'');
                    if (installedDirsSet.contains(pathLower)) {
                        int claimants = pathCounts.value(pathLower);
                        if (claimants == 1) {
                            mod.isInstalled = true;
                            mod.installPath = addonsDir.filePath(mod.addons[0].path);
                            matchedCount++;
                            pathMatch++;
                        }
                    }
                }
            }

            for (const QString& dir : installedDirs) {
                const QString dirLower = dir.toLower();
                bool alreadyMatched = false;
                for (const ModInfo& mod : m_mods) {
                    QString ip = mod.installPath.toLower();
                    ip.remove('\'');
                    QString dl = dir.toLower();
                    dl.remove('\'');
                    if (mod.isInstalled && ip.endsWith(dl)) {
                        alreadyMatched = true;
                        break;
                    }
                }
                if (alreadyMatched) continue;

                QList<int> candidates;
                for (int i = 0; i < m_mods.size(); ++i) {
                    const ModInfo& mod = m_mods[i];
                    if (!mod.isInstalled && !mod.addons.isEmpty()) {
                        QString mp = mod.addons[0].path;
                        mp.remove('\'');
                        if (mp.compare(dir, Qt::CaseInsensitive) == 0) {
                            candidates.append(i);
                        }
                    }
                }
                if (candidates.isEmpty()) continue;

                int bestIdx = -1;
                for (int i : candidates) {
                    const QString title = m_mods[i].title.toLower();
                    if (title == dirLower || QString(title).remove(' ') == QString(dirLower).remove(' ')) {
                        bestIdx = i;
                        break;
                    }
                }
                if (bestIdx < 0) {
                    for (int i : candidates) {
                        QString norm = m_mods[i].title.toLower();
                        norm.remove(' ');
                        QString dirNorm = dirLower;
                        dirNorm.remove(' ');
                        if (norm.startsWith(dirNorm)) {
                            if (norm.length() == dirNorm.length()) {
                                bestIdx = i; break;
                            }
                            QChar next = norm.at(dirNorm.length());
                            if (!next.isLetterOrNumber()) {
                                bestIdx = i; break;
                            }
                        }
                    }
                }

                if (bestIdx < 0)
                    bestIdx = candidates[0];

                if (bestIdx >= 0) {
                    m_mods[bestIdx].isInstalled = true;
                    m_mods[bestIdx].installPath = addonsDir.filePath(dir);
                    matchedCount++;
                    pathMatch++;
                }
            }

        int updateCount = 0;
        for (ModInfo& mod : m_mods) {
            mod.hasUpdate = false;
            if (!mod.isInstalled || mod.version.isEmpty())
                continue;

            if (!m_installedVersions.contains(mod.id))
                continue;
            if (m_installedVersions[mod.id] != mod.version) {
                mod.hasUpdate = true;
                updateCount++;
            }
        }

        if (updateCount > 0)
            spdlog::info("Update check: {} addon(s) have version changes", updateCount);

        if (matchedCount < installedDirs.size()) {
            QStringList unmatched;
            for (const QString& dir : installedDirs) {
                bool found = false;
                for (const ModInfo& mod : m_mods) {
                    if (mod.isInstalled) {
                        QString ip = mod.installPath.toLower();
                        ip.remove('\'');
                        QString dl = dir.toLower();
                        dl.remove('\'');
                        if (ip.endsWith(dl)) {
                            found = true;
                            break;
                        }
                    }
                }
                if (!found)
                    unmatched.append(dir);
            }
            unmatched.sort();
            for (const QString& dir : unmatched) {
                QStringList claimants;
                for (const ModInfo& mod : m_mods) {
                    if (!mod.addons.isEmpty()) {
                        QString mp = mod.addons[0].path;
                        mp.remove('\'');
                        if (mp.compare(dir, Qt::CaseInsensitive) == 0) {
                            claimants.append(mod.title);
                        }
                    }
                }
            }
        }

        });

    m_installedCheckWatcher.setFuture(checkFuture);
}

void Lexicon::onInstalledAddonsCheckFinished() {
    bool migrated = false;
    for (auto it = m_installedVersions.begin(); it != m_installedVersions.end(); ) {
        if (it.value().length() == 32 && !it.value().contains('.')) {
            it = m_installedVersions.erase(it);
            migrated = true;
        } else {
            ++it;
        }
    }
    if (migrated) {
        saveInstalledFolders();
    }

    int seeded = 0;
    bool needSave = false;
    for (const ModInfo& mod : m_mods) {
        if (mod.isInstalled && !mod.version.isEmpty() && !m_installedVersions.contains(mod.id)) {
            m_installedVersions[mod.id] = mod.version;
            needSave = true;
            seeded++;
        }
    }

    if (needSave)
        saveInstalledFolders();

    for (ModInfo& mod : m_mods) {
        mod.storedVersion.clear();
        if (m_installedVersions.contains(mod.id)) {
            mod.storedVersion = m_installedVersions[mod.id];
            if (mod.isInstalled)
                spdlog::info("  stored version: {} stored={} current={}", mod.title.toStdString(), mod.storedVersion.toStdString(), mod.version.toStdString());
        }
    }

    m_addonModel->setMods(m_mods);
    m_installedAddonsFilter->refreshFilter();

    m_availableUpdates = 0;
    for (const ModInfo& mod : m_mods) {
        if (mod.isInstalled && mod.hasUpdate)
            m_availableUpdates++;
    }
    emit availableUpdatesChanged();
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
                : QString();
            info.addonCount = it.value();
            categories.append(info);
        }

        // Sort categories alphabetically
        std::sort(categories.begin(), categories.end(), [](const CategoryInfo& a, const CategoryInfo& b) {
            return a.categoryName.compare(b.categoryName, Qt::CaseInsensitive) < 0;
        });

        CategoryInfo allInfo;
        allInfo.categoryId = QString();
        allInfo.categoryName = QStringLiteral("All");
        allInfo.iconSource = QString();
        allInfo.addonCount = m_mods.count();
        categories.prepend(allInfo);

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

                    QDir dir(addonsPath);
                    const QStringList afterDirs = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
                    trackInstalledFolders(modId, beforeDirs, afterDirs);

                    QFile::remove(tempFilePath);
                    onDone(true, QString());
                });

        QString command = QStringLiteral(
            "powershell -NoProfile -Command \"Expand-Archive -LiteralPath '%1' -DestinationPath '%2' -Force\"")
            .arg(QString(tempFilePath).replace('\'', "''"), QString(addonsPath).replace('\'', "''"));

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

    QSet<QString> depTitles;
    QStringList optionalTitles;
    static const QRegularExpression verSuffix(QStringLiteral("[><=]+[\\d.]+$"));
    for (const Dependencies& dep : mod->addons) {
        for (const QString& rawTitle : dep.requiredDependencies) {
            QString clean = QString(rawTitle).remove(verSuffix).trimmed();
            if (!clean.isEmpty()) depTitles.insert(clean);
        }
        for (const QString& rawTitle : dep.optionalDependencies) {
            QString clean = QString(rawTitle).remove(verSuffix).trimmed();
            if (!clean.isEmpty() && !depTitles.contains(clean)) optionalTitles.append(clean);
        }
    }
    optionalTitles.removeDuplicates();

    if (depTitles.isEmpty()) {
        if (!optionalTitles.isEmpty()) {
            QString modTitle;
            for (const ModInfo& m : m_mods) { if (m.id == modId) { modTitle = m.title; break; } }
            QTimer::singleShot(50, this, [this, modId, modTitle, optionalTitles]() {
                emit optionalDependenciesPrompt(modId, modTitle, optionalTitles);
            });
        } else {
            emit addonInstallFinished(modId);
        }
        refreshInstalledStatus();
        return;
    }

    struct DepEntry { QString id; QString title; QString downloadUrl; };
    QList<DepEntry> toInstall;

    for (const QString& depTitle : depTitles) {
        const QString lowerTitle = depTitle.toLower();
        const QString lowerTitleNoSpaces = QString(lowerTitle).remove(' ');
        bool found = false;

        // Pass 1: match by title
        for (const ModInfo& m : m_mods) {
            const QString mLower = m.title.toLower();
            QString mLowerNoSpaces = mLower;
            mLowerNoSpaces.remove(' ');
            if ((mLower == lowerTitle || mLowerNoSpaces == lowerTitleNoSpaces)
                && !m.isInstalled && m.id != modId) {
                toInstall.append({ m.id, m.title, m.downloadUrl.toString() });
                found = true;
                break;
            }
        }

        // Pass 2: match by addons[0].path (folder name in dependency's JSON)
        if (!found) {
            for (const ModInfo& m : m_mods) {
                if (m.isInstalled || m.id == modId || m.addons.isEmpty())
                    continue;
                const QString mPath = m.addons[0].path.toLower();
                if (mPath == lowerTitle || mPath == lowerTitleNoSpaces) {
                    toInstall.append({ m.id, m.title, m.downloadUrl.toString() });
                    found = true;
                    break;
                }
            }
        }

        // Pass 3: substring match against title
        if (!found && lowerTitleNoSpaces.length() >= 4) {
            for (const ModInfo& m : m_mods) {
                QString mLowerNoSpaces = m.title.toLower();
                mLowerNoSpaces.remove(' ');
                if (mLowerNoSpaces.contains(lowerTitleNoSpaces)
                    && !m.isInstalled && m.id != modId) {
                    toInstall.append({ m.id, m.title, m.downloadUrl.toString() });
                    found = true;
                    break;
                }
            }
        }

        if (!found) {
            spdlog::warn("installDependenciesFor: could not find mod for dependency '{}'", depTitle.toStdString());
        }
    }

    if (toInstall.isEmpty()) {
        if (!optionalTitles.isEmpty()) {
            QString modTitle;
            for (const ModInfo& m : m_mods) { if (m.id == modId) { modTitle = m.title; break; } }
            QTimer::singleShot(50, this, [this, modId, modTitle, optionalTitles]() {
                emit optionalDependenciesPrompt(modId, modTitle, optionalTitles);
            });
        } else {
            emit addonInstallFinished(modId);
        }
        refreshInstalledStatus();
        return;
    }

    emit addonInstallStatusChanged(modId, QStringLiteral("installing_dependencies"));

    auto sharedIdx = std::make_shared<int>(0);
    auto sharedList = std::make_shared<QList<DepEntry>>(toInstall);
    auto installNext = std::make_shared<std::function<void()>>();

    *installNext = [this, modId, sharedIdx, sharedList, installNext, optionalTitles]() {
        if (*sharedIdx >= sharedList->size()) {
            if (!optionalTitles.isEmpty()) {
                QString modTitle;
                for (const ModInfo& m : m_mods) { if (m.id == modId) { modTitle = m.title; break; } }
                QTimer::singleShot(50, this, [this, modId, modTitle, optionalTitles]() {
                    emit optionalDependenciesPrompt(modId, modTitle, optionalTitles);
                });
            } else {
                emit addonInstallFinished(modId);
            }
            refreshInstalledStatus();
            return;
        }

        const DepEntry& dep = sharedList->at(*sharedIdx);
        emit addonDependencyInstallStarted(modId, dep.title);

        downloadAndExtractAddon(dep.id, dep.title, dep.downloadUrl,
            [this, modId, sharedIdx, installNext](bool success, const QString& error) {
                if (!success) {
                    spdlog::warn("installAddon: dependency failed: {}", error.toStdString());
                }
                (*sharedIdx)++;
                (*installNext)();
            });
    };

    (*installNext)();
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
        m_installedVersions.remove(modId);
        saveInstalledFolders();

        QDir addonsDir(addonsPath);
        for (const QString& folder : folders) {
            const QString fullPath = addonsDir.filePath(folder);
            QDir(fullPath).removeRecursively();
        }
        emit addonUninstallFinished(modId);
        refreshInstalledStatus();
        return;
    }

    // Fallback: find the mod and match by addons[0].path or title
    const ModInfo* mod = nullptr;
    for (const ModInfo& m : m_mods) {
        if (m.id == modId) { mod = &m; break; }
    }

    QDir addonsDir(addonsPath);
    if (!addonsDir.exists()) {
        spdlog::warn("uninstallAddon: Addons directory does not exist");
        return;
    }

    const QStringList dirs = addonsDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);

    if (mod && !mod->addons.isEmpty()) {
        QString pathName = mod->addons[0].path;
        pathName.remove('\'');
        for (const QString& dir : dirs) {
            QString d = dir;
            d.remove('\'');
            if (d.compare(pathName, Qt::CaseInsensitive) == 0) {
                const QString fullPath = addonsDir.filePath(dir);
                QDir(fullPath).removeRecursively();
                m_installedVersions.remove(modId);
                saveInstalledFolders();
                emit addonUninstallFinished(modId);
                refreshInstalledStatus();
                return;
            }
        }
    }

    const QString lowerTitle = title.toLower();
    const QString lowerTitleNoSpaces = QString(lowerTitle).remove(' ').remove('\'');
    static const QRegularExpression versionSuffix2(QStringLiteral("[\\d.]+$"));
    QString strippedTitle = lowerTitleNoSpaces;
    strippedTitle.remove(versionSuffix2);

    for (const QString& dir : dirs) {
        QString lowerDir = dir.toLower();
        if (lowerDir == lowerTitle || lowerDir == lowerTitleNoSpaces || lowerDir == strippedTitle) {
            const QString fullPath = addonsDir.filePath(dir);
            QDir(fullPath).removeRecursively();
            m_installedVersions.remove(modId);
            saveInstalledFolders();
            emit addonUninstallFinished(modId);
            refreshInstalledStatus();
            return;
        }
        QString lowerDirNoSpaces = lowerDir;
        lowerDirNoSpaces.remove(' ');
        lowerDirNoSpaces.remove('\'');
        if (lowerDirNoSpaces == lowerTitleNoSpaces || lowerDirNoSpaces == strippedTitle) {
            const QString fullPath = addonsDir.filePath(dir);
            QDir(fullPath).removeRecursively();
            m_installedVersions.remove(modId);
            saveInstalledFolders();
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

void Lexicon::cleanUnusedLibraries()
{
    const QString addonsPath = Pathing::getInstance()->paths().addons;
    if (addonsPath.isEmpty()) { return; }

    QList<LibInfo> installedLibs;
    for (const ModInfo& mod : m_mods) {
        if (!mod.isInstalled) continue;
        if (mod.title.startsWith(QStringLiteral("Lib"), Qt::CaseInsensitive) || mod.library)
            installedLibs.append({mod.id, mod.title});
    }
    if (installedLibs.isEmpty()) { emit cleanLibrariesFinished(0); return; }

    QSet<QString> neededLibIds;
    static const QRegularExpression verSuffix(QStringLiteral("[><=]+[\\d.]+$"));
    for (const ModInfo& mod : m_mods) {
        if (!mod.isInstalled) continue;
        if (mod.title.startsWith(QStringLiteral("Lib"), Qt::CaseInsensitive) || mod.library) continue;
        for (const Dependencies& dep : mod.addons) {
            auto resolveAndInsert = [&](const QString& raw) {
                QString depTitle = QString(raw).remove(verSuffix).trimmed();
                if (depTitle.isEmpty()) return;
                QString lt = depTitle.toLower(), ltns = QString(lt).remove(' ');
                // Pass 1: exact title match on installed library mods
                for (const ModInfo& m : m_mods) {
                    if (!m.isInstalled) continue;
                    if (!(m.title.startsWith("Lib", Qt::CaseInsensitive) || m.library)) continue;
                    if (m.title.toLower() == lt || QString(m.title.toLower()).remove(' ') == ltns)
                        { neededLibIds.insert(m.id); return; }
                }
                // Pass 2: match by addons[0].path (library mods only)
                for (const ModInfo& m : m_mods) {
                    if (!m.isInstalled || m.addons.isEmpty()) continue;
                    if (!(m.title.startsWith("Lib", Qt::CaseInsensitive) || m.library)) continue;
                    if (m.addons[0].path.toLower() == lt || QString(m.addons[0].path.toLower()).remove(' ') == ltns)
                        { neededLibIds.insert(m.id); return; }
                }
                // Pass 3: substring match on library mods (min 4 chars)
                if (ltns.length() >= 4) {
                    for (const ModInfo& m : m_mods) {
                        if (!m.isInstalled) continue;
                        if (!(m.title.startsWith("Lib", Qt::CaseInsensitive) || m.library)) continue;
                        if (QString(m.title.toLower()).remove(' ').contains(ltns))
                            { neededLibIds.insert(m.id); return; }
                    }
                }
            };
            for (const QString& raw : dep.requiredDependencies) resolveAndInsert(raw);
            for (const QString& raw : dep.optionalDependencies) resolveAndInsert(raw);
        }
    }

    neededLibIds.insert("2625"); neededLibIds.insert("3353"); neededLibIds.insert("4494");

    spdlog::info("clean: {} libs scanned, {} resolved dep IDs", installedLibs.size(), neededLibIds.size());
    m_pendingCleanup.clear();
    for (const LibInfo& lib : installedLibs) {
        if (!neededLibIds.contains(lib.id)) {
            spdlog::info("  UNUSED: {} (id={})", lib.title.toStdString(), lib.id.toStdString());
            m_pendingCleanup.append(lib);
        }
    }

    if (!m_pendingCleanup.isEmpty()) {
        QSet<QString> checkIds;
        for (const auto& l : m_pendingCleanup) checkIds.insert(l.id);
        for (auto it = m_pendingCleanup.begin(); it != m_pendingCleanup.end(); ) {
            bool protected_ = false;
            for (const ModInfo& mod : m_mods) {
                if (!mod.isInstalled || checkIds.contains(mod.id) || mod.id == it->id) continue;
                for (const Dependencies& dep : mod.addons) {
                    for (const QString& raw : dep.requiredDependencies)
                        if (it->title.compare(QString(raw).remove(verSuffix).trimmed(), Qt::CaseInsensitive) == 0)
                            { protected_ = true; goto done; }
                    for (const QString& raw : dep.optionalDependencies)
                        if (it->title.compare(QString(raw).remove(verSuffix).trimmed(), Qt::CaseInsensitive) == 0)
                            { protected_ = true; goto done; }
                }
                done: if (protected_) break;
            }
            if (protected_) { spdlog::info("  KEPT (lib-lib dep): {}", it->title.toStdString()); it = m_pendingCleanup.erase(it); }
            else ++it;
        }
    }
    if (m_pendingCleanup.isEmpty()) { emit cleanLibrariesFinished(0); return; }

    QStringList titles;
    for (const auto& lib : m_pendingCleanup)
        titles.append(lib.title);
    emit cleanLibrariesConfirm(titles);
}

void Lexicon::confirmCleanLibraries()
{
    if (m_pendingCleanup.isEmpty()) { emit cleanLibrariesFinished(0); return; }

    const QString addonsPath = Pathing::getInstance()->paths().addons;
    QDir addonsDir(addonsPath);
    for (int i = 0; i < m_pendingCleanup.size(); ++i) {
        const auto& lib = m_pendingCleanup[i];
        emit cleanLibrariesProgress(i + 1, m_pendingCleanup.size(), lib.title);

        if (m_installedFolders.contains(lib.id)) {
            for (const QString& folder : m_installedFolders.take(lib.id))
                QDir(addonsDir.filePath(folder)).removeRecursively();
            m_installedVersions.remove(lib.id);
        } else {
            uninstallAddon(lib.id, lib.title);
        }
    }
    saveInstalledFolders();
    refreshInstalledStatus();
    emit cleanLibrariesFinished(m_pendingCleanup.size());
    m_pendingCleanup.clear();
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
        const QJsonValue val = it.value();
        if (val.isObject()) {
            // format: { "f": [...], "t": "..." }
            const QJsonObject obj = val.toObject();
            const QJsonArray arr = obj["f"].toArray();
            QStringList folders;
            folders.reserve(arr.size());
            for (const QJsonValue& v : arr)
                folders.append(v.toString());
            if (!folders.isEmpty())
                m_installedFolders[it.key()] = folders;
            QString ck = obj["c"].toString();
            if (ck.isEmpty()) ck = obj["t"].toString();
            if (ck.isEmpty()) ck = obj["v"].toString();
            if (!ck.isEmpty())
                m_installedVersions[it.key()] = ck;
        } else if (val.isArray()) {
            const QJsonArray arr = val.toArray();
            QStringList folders;
            folders.reserve(arr.size());
            for (const QJsonValue& v : arr)
                folders.append(v.toString());
            if (!folders.isEmpty())
                m_installedFolders[it.key()] = folders;
        }
    }
}

void Lexicon::saveInstalledFolders()
{
    QJsonObject root;
    for (auto it = m_installedFolders.begin(); it != m_installedFolders.end(); ++it) {
        QJsonObject obj;
        QJsonArray arr;
        for (const QString& folder : it.value())
            arr.append(folder);
        obj["f"] = arr;
        if (m_installedVersions.contains(it.key()))
            obj["c"] = m_installedVersions[it.key()];
        root[it.key()] = obj;
    }

    for (auto it = m_installedVersions.begin(); it != m_installedVersions.end(); ++it) {
        if (!root.contains(it.key())) {
            QJsonObject obj;
            obj["f"] = QJsonArray();
            obj["c"] = it.value();
            root[it.key()] = obj;
        }
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
        spdlog::info("trackInstalledFolders: no new folders for mod {} (update)", modId.toStdString());
    } else {
        m_installedFolders[modId] = newFolders;
    }

    for (const ModInfo& mod : m_mods) {
        if (mod.id == modId && !mod.version.isEmpty()) {
            m_installedVersions[modId] = mod.version;
            break;
        }
    }

    saveInstalledFolders();

    for (ModInfo& mod : m_mods) {
        if (mod.id == modId) {
            mod.hasUpdate = false;
            mod.storedVersion = mod.version;
        }
    }
    m_addonModel->setMods(m_mods);
    m_installedAddonsFilter->refreshFilter();
}

void Lexicon::fetchAddonDescription(const QString& fileInfoUrl)
{
    if (m_descriptionFetcher)
        m_descriptionFetcher->fetch(fileInfoUrl);
}

QString Lexicon::addonsPath() const
{
    return Pathing::getInstance()->paths().addons;
}

void Lexicon::setAddonsPath(const QString& path)
{
    Pathing::getInstance()->setAddonsPath(path);
}

int Lexicon::availableUpdates() const
{
    return m_availableUpdates;
}

QString Lexicon::getInstalledVersionForAddon(const QString& modId) const
{
    if (m_installedVersions.contains(modId))
        return m_installedVersions[modId];
    return QString();
}

QString Lexicon::appVersion() const
{
    return QCoreApplication::applicationVersion();
}

bool Lexicon::appUpdateAvailable() const
{
    return m_appUpdateAvailable;
}

QString Lexicon::appUpdateVersion() const
{
    return m_appUpdateVersion;
}

void Lexicon::checkForAppUpdate()
{
    QNetworkAccessManager* mgr = new QNetworkAccessManager(this);
    QUrl url(QStringLiteral("https://raw.githubusercontent.com/mojabed/ModdingLexicon/master/version.json?t=%1").arg(QDateTime::currentSecsSinceEpoch()));
    QNetworkRequest req(url);
    req.setRawHeader("User-Agent", "ModdingLexicon/1.0");
    req.setTransferTimeout(10000);

    QNetworkReply* reply = mgr->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply, mgr]() {
        reply->deleteLater();
        mgr->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            spdlog::debug("App update check failed: {}", reply->errorString().toStdString());
            return;
        }

        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        if (!doc.isObject()) return;
        QJsonObject obj = doc.object();
        QString latest = obj["version"].toString();
        QString downloadUrl = obj["url"].toString();
        QString current = appVersion();
        spdlog::info("App update check: latest='{}' current='{}'", latest.toStdString(), current.toStdString());
        if (latest.isEmpty() || downloadUrl.isEmpty() || latest == current) return;

        m_appUpdateVersion = latest;
        m_appUpdateDownloadUrl = downloadUrl;
        m_appUpdateAvailable = true;
        emit appUpdateAvailableChanged();
        spdlog::info("App update available: {} from {}", latest.toStdString(), downloadUrl.toStdString());
    });
}

void Lexicon::downloadAppUpdate()
{
    if (m_appUpdateDownloadUrl.isEmpty()) return;

    QString installerPath = QDir::tempPath() + QStringLiteral("/ModdingLexicon_Update.exe");
    QNetworkAccessManager* mgr = new QNetworkAccessManager(this);
    QUrl url(m_appUpdateDownloadUrl);
    QNetworkRequest updateReq(url);
    updateReq.setRawHeader("User-Agent", "ModdingLexicon/1.0");
    updateReq.setTransferTimeout(60000);

    QNetworkReply* updateReply = mgr->get(updateReq);
    connect(updateReply, &QNetworkReply::finished, this, [this, updateReply, mgr, installerPath]() {
        updateReply->deleteLater();
        mgr->deleteLater();

        if (updateReply->error() != QNetworkReply::NoError) {
            spdlog::error("App update download failed: {}", updateReply->errorString().toStdString());
            return;
        }

        QFile file(installerPath);
        if (file.open(QIODevice::WriteOnly)) {
            file.write(updateReply->readAll());
            file.close();
            QProcess::startDetached(installerPath, QStringList());
            QCoreApplication::quit();
        }
    });
}

void Lexicon::installOptionalDependency(const QString& modId, const QString& depTitle)
{
    const QString lowerTitle = depTitle.toLower();
    const QString lowerTitleNoSpaces = QString(lowerTitle).remove(' ');
    for (const ModInfo& m : m_mods) {
        const QString mLower = m.title.toLower();
        QString mLowerNoSpaces = mLower; mLowerNoSpaces.remove(' ');
        if ((mLower == lowerTitle || mLowerNoSpaces == lowerTitleNoSpaces) && !m.isInstalled && !m.downloadUrl.isEmpty()) {
            downloadAndExtractAddon(m.id, m.title, m.downloadUrl.toString(),
                [this](bool success, const QString& error) {
                    if (!success) spdlog::warn("Optional dep failed: {}", error.toStdString());
                    refreshInstalledStatus();
                });
            return;
        }
    }
    spdlog::warn("installOptionalDependency: could not find '{}'", depTitle.toStdString());
}

void Lexicon::finishAddonInstall(const QString& modId)
{
    refreshInstalledStatus();
    emit addonInstallFinished(modId);
}
