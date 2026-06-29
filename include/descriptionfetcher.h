#pragma once

#include <QObject>
#include <QPair>
#include <QPointer>
#include <QString>
#include <QList>

class QNetworkAccessManager;
class QNetworkReply;

class DescriptionFetcher : public QObject {
    Q_OBJECT
public:
    explicit DescriptionFetcher(QObject* parent = nullptr);
    ~DescriptionFetcher();

    void fetch(const QString& fileInfoUrl);
    void cancel();
    void pruneImageCache(const QString& cacheDir);

signals:
    void descriptionReady(const QString& fileInfoUrl, const QString& description);

private:
    void lazyLoadImages(const QString& fileInfoUrl,
                        const QString& baseDescription,
                        const QList<QPair<QString, QString>>& imagePairs);

    QNetworkAccessManager* m_nam = nullptr;
    QPointer<QNetworkReply> m_currentReply;
    QList<QPointer<QNetworkReply>> m_pendingImageReplies;
};
