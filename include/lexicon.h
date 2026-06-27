#pragma once

#include <QObject>
#include <QtQml>
#include <QDateTime>
#include <QFutureWatcher>
#include <QMap>

#include "ModType.h"
#include "CategoryModel.h"

class HttpClient;
class AddonModel;
class AddonFilterModel;

class Lexicon : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_NAMED_ELEMENT(Lexicon)
    Q_PROPERTY(AddonModel* addonModel READ addonModel CONSTANT)
    Q_PROPERTY(AddonFilterModel* installedAddonsFilter READ installedAddonsFilter CONSTANT)
    Q_PROPERTY(CategoryModel* categoryModel READ categoryModel CONSTANT)

public:
    explicit Lexicon(QObject* parent = nullptr);
    ~Lexicon();

    AddonModel* addonModel() const;
    AddonFilterModel* installedAddonsFilter() const;

    CategoryModel* categoryModel() const;

signals:
    void masterListReady(const QString& filePath);
    void downloadError(const QString& message);

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
    bool loadCachedMasterList();

    QString m_masterListPath;
    QString m_categoryListPath;
    QList<ModInfo> m_mods;
    QMap<QString, QString> m_categoryNames;
    QMap<QString, QString> m_categoryIcons;

    QFutureWatcher<QList<ModInfo>> m_parsingWatcher;
    QFutureWatcher<void> m_installedCheckWatcher;
};