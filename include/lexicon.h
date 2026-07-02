#pragma once

#include <QObject>
#include <QtQml>
#include <QFutureWatcher>
#include <QMap>
#include <functional>

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
    Q_PROPERTY(QString gameVersion READ gameVersion NOTIFY gameVersionChanged)
    Q_PROPERTY(QString gameVersionName READ gameVersionName NOTIFY gameVersionChanged)
    Q_PROPERTY(int availableUpdates READ availableUpdates NOTIFY availableUpdatesChanged)
    Q_PROPERTY(QString addonsPath READ addonsPath CONSTANT)

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
    Q_INVOKABLE QString getGameVersionForAddon(const QString& modId) const;
    Q_INVOKABLE int getAddonApiVersion(const QString& modId) const;
    Q_INVOKABLE void refreshCategoryCounts();
    Q_INVOKABLE void setAddonsPath(const QString& path);
    QString currentDescription() const { return m_currentDescription; }
    QString gameVersion() const { return m_gameVersion; }
    QString gameVersionName() const { return m_gameVersionName; }
    QString addonsPath() const;
    int availableUpdates() const;

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
    void addonDependencyInstallStarted(const QString& modId, const QString& depTitle);
    void addonInstallStatusChanged(const QString& modId, const QString& status);
    void gameVersionChanged();
    void availableUpdatesChanged();

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
    void updateGameConfig();
    void parseMasterList();
    void parseCategoryList();
    void parseGameConfig();
    void applyGameVersionsToMods();
    void checkInstalledAddons();

    void loadInstalledFolders();
    void saveInstalledFolders();
    void trackInstalledFolders(const QString& modId, const QStringList& before, const QStringList& after);
    void downloadAndExtractAddon(const QString& modId, const QString& title,
                                 const QString& downloadUrl,
                                 std::function<void(bool, const QString&)> onDone);
    void installDependenciesFor(const QString& modId, const QString& addonsPath);

    QString m_masterListPath;
    QString m_categoryListPath;
    QString m_gameConfigPath;
    QString m_installedFoldersPath;
    QList<ModInfo> m_mods;
    QMap<QString, QString> m_categoryNames;
    QMap<QString, QString> m_categoryIcons;
    QMap<QString, QStringList> m_installedFolders;
    QMap<QString, QString> m_installedChecksums;  // modId -> api checksum at install
    QString m_discontinuedCategoryId;

    QFutureWatcher<QList<ModInfo>> m_parsingWatcher;
    QFutureWatcher<void> m_installedCheckWatcher;

    DescriptionFetcher* m_descriptionFetcher = nullptr;
    QString m_currentDescription;
    QString m_gameVersion;
    QString m_gameVersionName; 
    int m_availableUpdates = 0;
    QMap<int, QPair<QString, QString>> m_gameVersionMap; // apiVersion -> (version, name)
};
