#pragma once

#include "ModType.h"
#include <QList>

class QJsonArray;
class QJsonObject;
class QByteArray;

class Parser {
public:
    static ModInfo fromCacheJson(const QJsonObject& json);
    static QJsonObject toCacheJson(const ModInfo& mod);

    static ModInfo fromEsoJson(const QJsonObject& json);
    static QList<ModInfo> parseEsoMods(const QByteArray& jsonData);

private:
    // Example: https://www.esoui.com/downloads/download<id>-<title>
    static const QString m_downloadUrlPrefix;
};