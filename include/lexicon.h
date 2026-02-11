#pragma once

#include <QObject>
#include <QtQml>
#include <QDateTime>

#include "ModType.h"

class HttpClient;
class AddonModel;

class Lexicon : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_NAMED_ELEMENT(Lexicon)
    Q_PROPERTY(AddonModel* addonModel READ addonModel CONSTANT)

public:
    explicit Lexicon(QObject* parent = nullptr);
    ~Lexicon();

    AddonModel* addonModel() const;

signals:
    void masterListReady(const QString& filePath);
    void downloadError(const QString& message);

private:
    HttpClient* m_httpClient;
    AddonModel* m_addonModel;

    void updateMasterList();
    void checkMasterListUpdate();
    void parseMasterList();
    bool loadCachedMasterList();

    QString m_masterListPath;
    QList<ModInfo> m_mods;
};