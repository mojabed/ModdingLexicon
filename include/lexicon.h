#pragma once

#include <QObject>
#include <QtQml>
#include <QDateTime>
#include <QFutureWatcher>

#include "ModType.h"

class HttpClient;
class AddonModel;
class AddonFilterModel;

class Lexicon : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_NAMED_ELEMENT(Lexicon)
    Q_PROPERTY(AddonModel* addonModel READ addonModel CONSTANT)
    Q_PROPERTY(AddonFilterModel* installedAddonsFilter READ installedAddonsFilter CONSTANT)

public:
    explicit Lexicon(QObject* parent = nullptr);
    ~Lexicon();

    AddonModel* addonModel() const;
    AddonFilterModel* installedAddonsFilter() const;

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

    void updateMasterList();
    void parseMasterList();
    void checkInstalledAddons();
    bool loadCachedMasterList();

    QString m_masterListPath;
    QList<ModInfo> m_mods;
    
    QFutureWatcher<QList<ModInfo>> m_parsingWatcher;
    QFutureWatcher<void> m_installedCheckWatcher;
};