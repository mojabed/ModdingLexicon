#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QMap>
#include <functional>

struct ModInfo;

class AddonInstaller : public QObject {
    Q_OBJECT
public:
    explicit AddonInstaller(QObject* parent = nullptr);

    void setAddonsPath(const QString& path);
    QString addonsPath() const;

    void install(const QString& modId, const QString& downloadUrl,
                 std::function<void(bool, const QString&)> onDone);
    bool uninstall(const QString& modId, const QString& title);

    void setModsPtr(QList<ModInfo>* mods);
    void setInstalledFoldersPtr(QMap<QString, QStringList>* folders);
    void setInstalledVersionsPtr(QMap<QString, QString>* versions);
    void setSaveCallback(std::function<void()> saveCallback);

signals:
    void installStarted(const QString& modId);
    void installProgress(const QString& modId, int percent);
    void installFinished(const QString& modId);
    void installFailed(const QString& modId, const QString& error);
    void dependencyInstallStarted(const QString& modId, const QString& depTitle);
    void installStatusChanged(const QString& modId, const QString& status);
    void uninstallFinished(const QString& modId);
    void optionalDependenciesPrompt(const QString& modId, const QString& modTitle, const QStringList& depTitles);

private:
    void downloadAndExtract(const QString& modId, const QString& downloadUrl,
                            std::function<void(bool, const QString&)> onDone);
    void installDependencies(const QString& modId);
    void trackFolders(const QString& modId, const QStringList& before, const QStringList& after);

    QString m_addonsPath;
    QList<ModInfo>* m_mods = nullptr;
    QMap<QString, QStringList>* m_installedFolders = nullptr;
    QMap<QString, QString>* m_installedVersions = nullptr;
    std::function<void()> m_saveCallback;
};
