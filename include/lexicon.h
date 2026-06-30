#pragma once

#include <QObject>
#include <QtQml>
#include <QFutureWatcher>
#include <QMap>

#include "ModType.h"
#include "CategoryModel.h"

class HttpClient;
class AddonModel;
class AddonFilterModel;
class DescriptionFetcher;

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
    Q_INVOKABLE void installAddon(const QString& modId, const QString& title, const QString& downloadUrl);
    Q_INVOKABLE void uninstallAddon(const QString& modId, const QString& title);
    Q_INVOKABLE void refreshInstalledStatus();
    QString currentDescription() const { return m_currentDescription; }

signals:
    void masterListReady(const QString& filePath);
    void downloadError(const QString& message);
    void addonDescriptionReady(const QString& url, const QString& description);
    void currentDescriptionChanged();
    void addonInstallStarted(const QString& modId);
    void addonInstallProgress(const QString& modId, int percent);
    void addonInstallFinished(const QString& modId);
    void addonInstallFailed(const QString& modId, const QString& error);
    void addonUninstallFinished(const QString& modId);

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

    void loadInstalledFolders();
    void saveInstalledFolders();
    void trackInstalledFolders(const QString& modId, const QStringList& before, const QStringList& after);

    QString m_masterListPath;
    QString m_categoryListPath;
    QString m_installedFoldersPath;
    QList<ModInfo> m_mods;
    QMap<QString, QString> m_categoryNames;
    QMap<QString, QString> m_categoryIcons;
    QMap<QString, QStringList> m_installedFolders;
    QString m_discontinuedCategoryId;

    QFutureWatcher<QList<ModInfo>> m_parsingWatcher;
    QFutureWatcher<void> m_installedCheckWatcher;

    DescriptionFetcher* m_descriptionFetcher = nullptr;
    QString m_currentDescription;
};