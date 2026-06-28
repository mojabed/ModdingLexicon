#include <spdlog/spdlog.h>
#include <QStandardPaths>
#include <QDir>
#include <QFileInfo>
#include <chrono>
#include <QtConcurrent>
#include <QMap>
#include <QFile>
#include <algorithm>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <QPointer>
#include <QRegularExpression>
#include <QSharedPointer>

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
    m_descriptionFetcher = new QNetworkAccessManager(this);

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
            spdlog::debug("Installed dir: '{}'", dir.toStdString());
        }

        int matchedCount = 0;
        for (ModInfo& mod : m_mods) {
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
                        spdlog::debug("Normalized match: title='{}' -> dir='{}'",
                                      mod.title.toStdString(), dir.toStdString());
                        break;
                    }
                }
            }
        }

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

static QString extractDivByClass(const QString& html, const QString& className)
{
    QString search = "class=\"" + className + "\"";
    int start = html.indexOf(search, 0, Qt::CaseInsensitive);
    if (start == -1) {
        search = "class='" + className + "'";
        start = html.indexOf(search, 0, Qt::CaseInsensitive);
    }
    if (start == -1)
        return {};

    start = html.indexOf('>', start);
    if (start == -1)
        return {};
    ++start;

    int depth = 1;
    int pos = start;
    while (pos < html.length() && depth > 0) {
        int open = html.indexOf("<div", pos, Qt::CaseInsensitive);
        int close = html.indexOf("</div", pos, Qt::CaseInsensitive);

        if (close == -1)
            break;

        if (open != -1 && open < close) {
            ++depth;
            pos = open + 4;
        } else {
            --depth;
            if (depth == 0)
                return html.mid(start, close - start).trimmed();
            pos = close + 6;
        }
    }
    return {};
}

static QString stripTags(const QString& html)
{
    QString out;
    out.reserve(html.size());
    bool inTag = false;
    for (const QChar& ch : html) {
        if (ch == u'<')
            inTag = true;
        else if (ch == u'>')
            inTag = false;
        else if (!inTag)
            out += ch;
    }
    return out.trimmed();
}

static QString extractDescription(const QString& html)
{
    static const char* descMarkers[] = {
        "<h2>Description</h2>", "<h3>Description</h3>",
        "<h2>About</h2>", "<h3>About</h3>",
        ">Description<", ">Description:</"
    };
    for (const char* marker : descMarkers) {
        int idx = html.indexOf(QLatin1StringView(marker), 0, Qt::CaseInsensitive);
        if (idx != -1) {
            int start = html.indexOf(QLatin1Char('>'), idx) + 1;
            int end = html.length();
            static const char* stoppers[] = {
                "<h2", "<h3", "<h4", "<hr", "download",
                "file-info", "fileinfo", "changelog", "comments"
            };
            for (const char* stop : stoppers) {
                int s = html.indexOf(QLatin1StringView(stop), start, Qt::CaseInsensitive);
                if (s != -1 && s < end)
                    end = s;
            }
            if (end - start > 100)
                return html.mid(start, end - start).trimmed();
        }
    }

    static const char* classes[] = {
        "postmessage", "esoui-description", "description",
        "mod-description", "addon-description"
    };
    for (const char* cls : classes) {
        QString desc = extractDivByClass(html, QString::fromLatin1(cls));
        if (!desc.isEmpty())
            return desc;
    }

    QString out;
    out.reserve(html.size());
    int i = 0;
    while (i < html.size()) {
        if (html.at(i) == u'<' && i + 6 < html.size()) {
            QChar c1 = html.at(i + 1);
            QChar c2 = html.at(i + 2);
            if ((c1 == u's' || c1 == u'S') && (c2 == u'c' || c2 == u'C')) {
                int end = html.indexOf(QLatin1StringView("</script>"), i + 7, Qt::CaseInsensitive);
                if (end != -1) {
                    out += u' ';
                    i = end + 9;
                    continue;
                }
            } else if ((c1 == u's' || c1 == u'S') && (c2 == u't' || c2 == u'T')) {
                int end = html.indexOf(QLatin1StringView("</style>"), i + 6, Qt::CaseInsensitive);
                if (end != -1) {
                    out += u' ';
                    i = end + 8;
                    continue;
                }
            }
        }
        out += html.at(i);
        ++i;
    }

    QString trimmed = out.trimmed();
    return trimmed.size() > 100 ? trimmed : QString();
}

void Lexicon::fetchAddonDescription(const QString& fileInfoUrl)
{
    if (fileInfoUrl.isEmpty() || !m_descriptionFetcher)
        return;

    if (m_currentDescriptionReply) {
        m_currentDescriptionReply->abort();
        m_currentDescriptionReply = nullptr;
    }

    m_currentDescription.clear();
    emit currentDescriptionChanged();

    QTimer::singleShot(0, this, [this, fileInfoUrl]() {
        QUrl qurl(fileInfoUrl);
        QNetworkRequest req(qurl);
        req.setTransferTimeout(15000);
        req.setRawHeader("User-Agent", "ModdingLexicon/1.0");

        QNetworkReply* reply = m_descriptionFetcher->get(req);
        m_currentDescriptionReply = reply;

        connect(reply, &QNetworkReply::finished, this, [this, reply, fileInfoUrl]() {
            reply->deleteLater();

            if (m_currentDescriptionReply == reply)
                m_currentDescriptionReply = nullptr;

            if (reply->error() != QNetworkReply::NoError) {
                emit addonDescriptionReady(fileInfoUrl, QString());
                return;
            }

            const QString html = QString::fromUtf8(reply->readAll());

            QPointer<Lexicon> self(this);
            QtConcurrent::run([self, html, fileInfoUrl]() {
                if (!self)
                    return;

                QString desc = extractDescription(html);

                QStringList imageUrls;
                static const QRegularExpression imgSrcRe(
                    QStringLiteral("<img[^>]+src=[\"']([^\"']+)[\"']"),
                    QRegularExpression::CaseInsensitiveOption);
                auto imgIt = imgSrcRe.globalMatch(html);
                while (imgIt.hasNext()) {
                    auto m = imgIt.next();
                    QString src = m.captured(1).trimmed();
                    if (!src.isEmpty())
                        imageUrls.append(src);
                }

                static const QRegularExpression resourceTags(
                    QStringLiteral("<(img|video|audio|source|iframe|link)[^>]*>"),
                    QRegularExpression::CaseInsensitiveOption);
                desc.remove(resourceTags);

                QMetaObject::invokeMethod(self, [self, desc, fileInfoUrl, imageUrls]() {
                    if (!self)
                        return;
                    self->m_currentDescription = desc;
                    emit self->currentDescriptionChanged();
                    emit self->addonDescriptionReady(fileInfoUrl, desc);

                    if (!imageUrls.isEmpty())
                        self->lazyLoadDescriptionImages(fileInfoUrl, desc, imageUrls);
                }, Qt::QueuedConnection);
            });
        });
    });
}
void Lexicon::lazyLoadDescriptionImages(const QString& fileInfoUrl,
                                        const QString& baseDescription,
                                        const QStringList& imageUrls)
{
    const QString cacheDir = Pathing::getInstance()->paths().appData + "/image_cache";
    QDir().mkpath(cacheDir);

    struct DownloadCtx {
        QMap<QString, QString> urlToPath;
        int pending = 0;
        QString description;
        QString fileInfoUrl;
        QPointer<Lexicon> self;
    };

    auto ctx = QSharedPointer<DownloadCtx>::create();
    ctx->description = baseDescription;
    ctx->fileInfoUrl = fileInfoUrl;
    ctx->self = this;

    for (const QString& imgUrl : imageUrls) {
        QUrl resolved = QUrl(fileInfoUrl).resolved(QUrl(imgUrl));
        if (!resolved.isValid() || resolved.scheme().isEmpty())
            continue;

        QString cacheKey = QString::number(qHash(resolved.toString()));
        QString cachedPath = cacheDir + "/" + cacheKey;
        if (QFile::exists(cachedPath)) {
            ctx->urlToPath[imgUrl] = cachedPath;
            continue;
        }

        QNetworkRequest imgReq(resolved);
        imgReq.setTransferTimeout(10000);
        imgReq.setRawHeader("User-Agent", "ModdingLexicon/1.0");

        QNetworkReply* imgReply = m_descriptionFetcher->get(imgReq);
        ctx->pending++;

        connect(imgReply, &QNetworkReply::finished, this,
            [ctx, imgReply, imgUrl, cacheDir, cacheKey, cachedPath]() {
                imgReply->deleteLater();
                ctx->pending--;

                if (imgReply->error() == QNetworkReply::NoError) {
                    QFile file(cachedPath);
                    if (file.open(QIODevice::WriteOnly)) {
                        file.write(imgReply->readAll());
                        file.close();
                        ctx->urlToPath[imgUrl] = cachedPath;
                    }
                }

                if (ctx->pending == 0 && ctx->self) {
                    QString enriched = ctx->description;
                    for (auto it = ctx->urlToPath.begin();
                         it != ctx->urlToPath.end(); ++it) {
                        enriched += QStringLiteral("<br><img src=\"file:///%1\">")
                                        .arg(it.value());
                    }
                    ctx->self->m_currentDescription = enriched;
                    emit ctx->self->currentDescriptionChanged();
                    emit ctx->self->addonDescriptionReady(
                        ctx->fileInfoUrl, enriched);
                }
            });
    }

    if (ctx->pending == 0 && !ctx->urlToPath.isEmpty()) {
        QString enriched = ctx->description;
        for (auto it = ctx->urlToPath.begin(); it != ctx->urlToPath.end(); ++it) {
            enriched += QStringLiteral("<br><img src=\"file:///%1\">")
                            .arg(it.value());
        }
        m_currentDescription = enriched;
        emit currentDescriptionChanged();
        emit addonDescriptionReady(fileInfoUrl, enriched);
    }
}
