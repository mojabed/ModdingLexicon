#pragma once

#include <QObject>
#include <QtQml>
#include <QDateTime>
#include <QFutureWatcher>
#include <QMap>
#include <QPointer>

#include "ModType.h"
#include "CategoryModel.h"

class HttpClient;
class AddonModel;
class AddonFilterModel;
class QNetworkAccessManager;
class QNetworkReply;

class Lexicon : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_NAMED_ELEMENT(Lexicon)
    Q_PROPERTY(AddonModel* addonModel READ addonModel CONSTANT)
    Q_PROPERTY(AddonFilterModel* installedAddonsFilter READ installedAddonsFilter CONSTANT)
    Q_PROPERTY(CategoryModel* categoryModel READ categoryModel CONSTANT)
    Q_PROPERTY(QString currentDescription READ currentDescription NOTIFY currentDescriptionChanged)

public:
    explicit Lexicon(QObject* parent = nullptr);
    ~Lexicon();

    AddonModel* addonModel() const;
    AddonFilterModel* installedAddonsFilter() const;

    CategoryModel* categoryModel() const;

    Q_INVOKABLE void fetchAddonDescription(const QString& url);
    QString currentDescription() const { return m_currentDescription; }

signals:
    void masterListReady(const QString& filePath);
    void downloadError(const QString& message);
    void addonDescriptionReady(const QString& url, const QString& description);
    void currentDescriptionChanged();

private slots:
    void onParsingFinished();
    void onInstalledAddonsCheckFinished();

private:
    HttpClient* m_httpClient;
    AddonModel* m_addonModel;
    AddonFilterModel* m_installedAddonsFilter;

    CategoryModel* m_categoryModel;

    void populateCategories();
    void applyCategoryMetadataToMods();
    void updateMasterList();
    void updateCategoryList();
    void parseMasterList();
    void parseCategoryList();
    void checkInstalledAddons();
     void lazyLoadDescriptionImages(const QString& fileInfoUrl, const QString& baseDescription, const QStringList& imageUrls);
   bool loadCachedMasterList();

    QString m_masterListPath;
    QString m_categoryListPath;
    QList<ModInfo> m_mods;
    QMap<QString, QString> m_categoryNames;
    QMap<QString, QString> m_categoryIcons;
    QString m_discontinuedCategoryId;

    QFutureWatcher<QList<ModInfo>> m_parsingWatcher;
    QFutureWatcher<void> m_installedCheckWatcher;

    QNetworkAccessManager* m_descriptionFetcher = nullptr;
    QPointer<QNetworkReply> m_currentDescriptionReply;
    QString m_currentDescription;
};