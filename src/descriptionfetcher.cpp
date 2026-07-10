#include "descriptionfetcher.h"
#include "descriptionparser.h"
#include "pathing.h"

#include <QDir>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QRegularExpression>
#include <QSet>
#include <QTimer>
#include <QtConcurrent>
#include <functional>

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

                QString postmessageHtml = extractDivByClass(html, QStringLiteral("postmessage"));
                QString fileinfoPicsHtml = extractDivByClass(html, QStringLiteral("fileinfo-pics"));
                QString combinedImagesHtml = postmessageHtml + fileinfoPicsHtml;

                QList<QPair<QString, QString>> imagePairs;
                QSet<QString> seenThumbs;

                static const QRegularExpression anchorImgRe(
                    QStringLiteral("<a[^>]+href=[\"']([^\"']+)[\"'][^>]*>\\s*<img[^>]+src=[\"']([^\"']+)[\"']"),
                    QRegularExpression::CaseInsensitiveOption);
                auto anchorIt = anchorImgRe.globalMatch(combinedImagesHtml);
                while (anchorIt.hasNext()) {
                    auto m = anchorIt.next();
                    QString fullUrl = m.captured(1).trimmed();
                    QString thumbUrl = m.captured(2).trimmed();
                    if (!thumbUrl.isEmpty() && !seenThumbs.contains(thumbUrl)) {
                        seenThumbs.insert(thumbUrl);
                        imagePairs.append({thumbUrl, fullUrl.isEmpty() ? thumbUrl : fullUrl});
                    }
                }

                static const QRegularExpression dataSrcRe(
                    QStringLiteral("<img[^>]+src=[\"']([^\"']+)[\"'][^>]+(?:data-src|data-full|data-original)=[\"']([^\"']+)[\"']"),
                    QRegularExpression::CaseInsensitiveOption);
                auto dataIt = dataSrcRe.globalMatch(combinedImagesHtml);
                while (dataIt.hasNext()) {
                    auto m = dataIt.next();
                    QString thumbUrl = m.captured(1).trimmed();
                    QString fullUrl = m.captured(2).trimmed();
                    if (!thumbUrl.isEmpty() && !seenThumbs.contains(thumbUrl)) {
                        seenThumbs.insert(thumbUrl);
                        imagePairs.append({thumbUrl, fullUrl});
                    }
                }

                if (imagePairs.isEmpty()) {
                    static const QRegularExpression imgSrcRe(
                        QStringLiteral("<img[^>]+src=[\"']([^\"']+)[\"']"),
                        QRegularExpression::CaseInsensitiveOption);
                    auto imgIt = imgSrcRe.globalMatch(combinedImagesHtml);
                    while (imgIt.hasNext()) {
                        auto m = imgIt.next();
                        QString src = m.captured(1).trimmed();
                        if (!src.isEmpty() && !seenThumbs.contains(src)) {
                            seenThumbs.insert(src);
                            imagePairs.append({src, src});
                        }
                    }
                }

                static const QRegularExpression resourceTags(
                    QStringLiteral("<(img|video|audio|source|iframe|link)[^>]*>"),
                    QRegularExpression::CaseInsensitiveOption);
                desc.remove(resourceTags);

                QMetaObject::invokeMethod(self, [self, desc, fileInfoUrl, imagePairs]() {
                    if (!self)
                        return;
                    emit self->descriptionReady(fileInfoUrl, desc);
                    if (!imagePairs.isEmpty())
                        self->lazyLoadImages(fileInfoUrl, desc, imagePairs);
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
                                         const QList<QPair<QString, QString>>& imagePairs)
{
    const QString cacheDir = Pathing::getInstance()->paths().appData + "/image_cache";
    QDir().mkpath(cacheDir);

    static QHash<QString, QString> s_memCache;

    struct DownloadCtx {
        QMap<QString, QPair<QString, QString>> urlToPath;
        int pending = 0;
        QString description;
        QString fileInfoUrl;
        QPointer<DescriptionFetcher> self;
    };

    auto ctx = QSharedPointer<DownloadCtx>::create();
    ctx->description = baseDescription;
    ctx->fileInfoUrl = fileInfoUrl;
    ctx->self = this;

    std::function<void()> emitDescription = [ctx, cacheDir, fileInfoUrl, imagePairs]() {
        if (!ctx->self)
            return;

        ctx->urlToPath.clear();
        for (const auto& pair : imagePairs) {
            const QString& thumbUrl = pair.first;
            const QString& fullUrl = pair.second;

            QUrl resolvedThumb = QUrl(fileInfoUrl).resolved(QUrl(thumbUrl));
            QString thumbCacheKey = QString::number(qHash(resolvedThumb.toString()));
            QString thumbCachedPath = cacheDir + "/" + thumbCacheKey;
            QString thumbLocalPath;
            if (s_memCache.contains(resolvedThumb.toString()))
                thumbLocalPath = s_memCache[resolvedThumb.toString()];
            else if (QFile::exists(thumbCachedPath)) {
                thumbLocalPath = thumbCachedPath;
                s_memCache[resolvedThumb.toString()] = thumbCachedPath;
            }
            if (thumbLocalPath.isEmpty())
                continue;

            QString fullLocalPath = thumbLocalPath;
            if (fullUrl != thumbUrl) {
                QUrl resolvedFull = QUrl(fileInfoUrl).resolved(QUrl(fullUrl));
                QString fullCacheKey = QString::number(qHash(resolvedFull.toString()));
                QString fullCachedPath = cacheDir + "/" + fullCacheKey;
                if (s_memCache.contains(resolvedFull.toString()))
                    fullLocalPath = s_memCache[resolvedFull.toString()];
                else if (QFile::exists(fullCachedPath)) {
                    fullLocalPath = fullCachedPath;
                    s_memCache[resolvedFull.toString()] = fullCachedPath;
                } else {
                    continue;
                }
            }

            ctx->urlToPath[thumbUrl] = {thumbLocalPath, fullLocalPath};
        }

        if (ctx->urlToPath.isEmpty())
            return;

        QString enriched = ctx->description;
        enriched += QStringLiteral("<br>");
        for (auto it = ctx->urlToPath.begin(); it != ctx->urlToPath.end(); ++it) {
            const auto& paths = it.value();
            enriched += QStringLiteral("<a href=\"file:///%1\"><img src=\"file:///%2\"></a> ")
                            .arg(paths.second, paths.first);
        }
        emit ctx->self->descriptionReady(ctx->fileInfoUrl, enriched);
    };

    auto downloadImage = [this, ctx, cacheDir, &emitDescription](const QString& url,
                                                const QString& cacheKey,
                                                const QString& cachedPath) {
        QUrl resolved = QUrl(ctx->fileInfoUrl).resolved(QUrl(url));
        if (!resolved.isValid() || resolved.scheme().isEmpty())
            return false;

        if (s_memCache.contains(resolved.toString()))
            return true;

        if (QFile::exists(cachedPath)) {
            s_memCache[resolved.toString()] = cachedPath;
            return true;
        }

        QNetworkRequest imgReq(resolved);
        imgReq.setTransferTimeout(10000);
        imgReq.setRawHeader("User-Agent", "ModdingLexicon/1.0");

        QNetworkReply* imgReply = m_nam->get(imgReq);
        m_pendingImageReplies.append(imgReply);
        ctx->pending++;

        connect(imgReply, &QNetworkReply::finished, this,
            [ctx, imgReply, cacheDir, cacheKey, cachedPath, resolved, emitDescription]() {
                imgReply->deleteLater();
                ctx->pending--;

                if (ctx->self)
                    ctx->self->m_pendingImageReplies.removeOne(imgReply);

                if (imgReply->error() == QNetworkReply::NoError) {
                    QFile file(cachedPath);
                    if (file.open(QIODevice::WriteOnly)) {
                        file.write(imgReply->readAll());
                        file.close();
                        s_memCache[resolved.toString()] = cachedPath;

                        if (ctx->self)
                            ctx->self->pruneImageCache(cacheDir);
                    }
                }

                if (ctx->pending == 0 && ctx->self)
                    emitDescription();
            });
        return false;
    };

    for (const auto& pair : imagePairs) {
        const QString& thumbUrl = pair.first;
        const QString& fullUrl = pair.second;

        QString thumbCacheKey = QString::number(qHash(QUrl(fileInfoUrl).resolved(QUrl(thumbUrl)).toString()));
        QString thumbCachedPath = cacheDir + "/" + thumbCacheKey;

        QUrl resolvedThumb = QUrl(fileInfoUrl).resolved(QUrl(thumbUrl));
        if (!s_memCache.contains(resolvedThumb.toString()) && !QFile::exists(thumbCachedPath))
            downloadImage(thumbUrl, thumbCacheKey, thumbCachedPath);

        if (fullUrl != thumbUrl) {
            QString fullCacheKey = QString::number(qHash(QUrl(fileInfoUrl).resolved(QUrl(fullUrl)).toString()));
            QString fullCachedPath = cacheDir + "/" + fullCacheKey;

            QUrl resolvedFull = QUrl(fileInfoUrl).resolved(QUrl(fullUrl));
            if (!s_memCache.contains(resolvedFull.toString()) && !QFile::exists(fullCachedPath))
                downloadImage(fullUrl, fullCacheKey, fullCachedPath);
        }
    }

    if (ctx->pending == 0)
        emitDescription();
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
