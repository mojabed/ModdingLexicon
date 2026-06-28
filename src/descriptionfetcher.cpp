#include "descriptionfetcher.h"
#include "descriptionparser.h"
#include "pathing.h"

#include <QDir>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QRegularExpression>
#include <QTimer>
#include <QtConcurrent>

DescriptionFetcher::DescriptionFetcher(QObject* parent)
    : QObject(parent)
{
    m_nam = new QNetworkAccessManager(this);
}

DescriptionFetcher::~DescriptionFetcher()
{
    cancel();
}

void DescriptionFetcher::fetch(const QString& fileInfoUrl)
{
    cancel();

    if (fileInfoUrl.isEmpty() || !m_nam)
        return;

    QTimer::singleShot(0, this, [this, fileInfoUrl]() {
        QUrl qurl(fileInfoUrl);
        QNetworkRequest req(qurl);
        req.setTransferTimeout(15000);
        req.setRawHeader("User-Agent", "ModdingLexicon/1.0");

        QNetworkReply* reply = m_nam->get(req);
        m_currentReply = reply;

        connect(reply, &QNetworkReply::finished, this, [this, reply, fileInfoUrl]() {
            reply->deleteLater();

            if (m_currentReply == reply)
                m_currentReply = nullptr;

            if (reply->error() != QNetworkReply::NoError) {
                emit descriptionReady(fileInfoUrl, QString());
                return;
            }

            const QString html = QString::fromUtf8(reply->readAll());

            QPointer<DescriptionFetcher> self(this);
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

                // Phase 1: emit text-only description immediately
                QMetaObject::invokeMethod(self, [self, desc, fileInfoUrl, imageUrls]() {
                    if (!self)
                        return;
                    emit self->descriptionReady(fileInfoUrl, desc);

                    // Phase 2: load images in background
                    if (!imageUrls.isEmpty())
                        self->lazyLoadImages(fileInfoUrl, desc, imageUrls);
                }, Qt::QueuedConnection);
            });
        });
    });
}

void DescriptionFetcher::cancel()
{
    if (m_currentReply) {
        m_currentReply->abort();
        m_currentReply = nullptr;
    }
    for (QPointer<QNetworkReply>& r : m_pendingImageReplies) {
        if (r) {
            r->abort();
            r->deleteLater();
        }
    }
    m_pendingImageReplies.clear();
}

void DescriptionFetcher::lazyLoadImages(const QString& fileInfoUrl,
                                         const QString& baseDescription,
                                         const QStringList& imageUrls)
{
    const QString cacheDir = Pathing::getInstance()->paths().appData + "/image_cache";
    QDir().mkpath(cacheDir);

    static QHash<QString, QString> s_memCache;

    struct DownloadCtx {
        QMap<QString, QString> urlToPath;
        int pending = 0;
        QString description;
        QString fileInfoUrl;
        QPointer<DescriptionFetcher> self;
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

        if (s_memCache.contains(resolved.toString())) {
            ctx->urlToPath[imgUrl] = s_memCache[resolved.toString()];
            continue;
        }
        if (QFile::exists(cachedPath)) {
            s_memCache[resolved.toString()] = cachedPath;
            ctx->urlToPath[imgUrl] = cachedPath;
            continue;
        }

        QNetworkRequest imgReq(resolved);
        imgReq.setTransferTimeout(10000);
        imgReq.setRawHeader("User-Agent", "ModdingLexicon/1.0");

        QNetworkReply* imgReply = m_nam->get(imgReq);
        m_pendingImageReplies.append(imgReply);
        ctx->pending++;

        connect(imgReply, &QNetworkReply::finished, this,
            [ctx, imgReply, imgUrl, cacheDir, cacheKey, cachedPath, resolved]() {
                imgReply->deleteLater();
                ctx->pending--;

                if (ctx->self)
                    ctx->self->m_pendingImageReplies.removeOne(imgReply);

                if (imgReply->error() == QNetworkReply::NoError) {
                    QFile file(cachedPath);
                    if (file.open(QIODevice::WriteOnly)) {
                        file.write(imgReply->readAll());
                        file.close();
                        ctx->urlToPath[imgUrl] = cachedPath;
                        s_memCache[resolved.toString()] = cachedPath;

                        if (ctx->self)
                            ctx->self->pruneImageCache(cacheDir);
                    }
                }

                if (ctx->pending == 0 && ctx->self) {
                    QString enriched = ctx->description;
                    for (auto it = ctx->urlToPath.begin();
                         it != ctx->urlToPath.end(); ++it) {
                        enriched += QStringLiteral("<br><img src=\"file:///%1\">")
                                        .arg(it.value());
                    }
                    emit ctx->self->descriptionReady(ctx->fileInfoUrl, enriched);
                }
            });
    }

    if (ctx->pending == 0 && !ctx->urlToPath.isEmpty()) {
        QString enriched = ctx->description;
        for (auto it = ctx->urlToPath.begin(); it != ctx->urlToPath.end(); ++it) {
            enriched += QStringLiteral("<br><img src=\"file:///%1\">")
                            .arg(it.value());
        }
        emit descriptionReady(fileInfoUrl, enriched);
    }
}

void DescriptionFetcher::pruneImageCache(const QString& cacheDir)
{
    static constexpr qint64 kMaxCacheBytes = 20LL * 1024 * 1024;

    QDir dir(cacheDir);
    QFileInfoList files = dir.entryInfoList(QDir::Files, QDir::Time | QDir::Reversed);

    qint64 totalSize = 0;
    for (const QFileInfo& fi : files)
        totalSize += fi.size();

    for (const QFileInfo& fi : files) {
        if (totalSize <= kMaxCacheBytes)
            break;
        qint64 sz = fi.size();
        if (QFile::remove(fi.absoluteFilePath()))
            totalSize -= sz;
    }
}
