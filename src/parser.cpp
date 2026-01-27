#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDateTime>
#include <QUrl>

#include "parser.h"

const QString Parser::m_downloadUrlPrefix = "https://www.esoui.com/downloads/download";

ModInfo Parser::fromEsoJson(const QJsonObject& json) {
    ModInfo mod;
    mod.isInstalled = false;

    mod.id = QString::number(json["id"].toDouble());
    mod.categoryId = json["categoryId"].isString() ? json["categoryId"].toString() : QString::number(json["categoryId"].toDouble());
    mod.version = json["version"].isString() ? json["version"].toString() : QString::number(json["version"].toDouble());

    mod.title = json["title"].toString();
    mod.author = json["author"].toString();
    mod.fileInfoUri = json["fileInfoUri"].toString();
    mod.downloads = json["downloads"].toInt();
    mod.downloadsMonthly = json["downloadsMonthly"].toInt();
    mod.checksum = json["checksum"].toString();
    mod.library = json["library"].toBool(false);
    mod.donationUrl = json.contains("donationUrl") ? QUrl(json["donationUrl"].toString()) : QUrl();

    if (json["lastUpdate"].isString()) {
        mod.lastUpdate = json["lastUpdate"].toString();
    } else {
        mod.lastUpdate = QString::number(json["lastUpdate"].toDouble());
    }

    QString sanitizedTitle = mod.title;
    sanitizedTitle.remove(' ');
    mod.downloadUrl = QUrl(m_downloadUrlPrefix + mod.id + "-" + sanitizedTitle);

    if (json.contains("gameVersions") && json["gameVersions"].isArray()) {
        QJsonArray versionsArray = json["gameVersions"].toArray();
        for (const QJsonValue& version : versionsArray) {
            mod.gameVersions.append(version.toString());
        }
    }

    if (json.contains("addons") && json["addons"].isArray()) {
        QJsonArray addonsArray = json["addons"].toArray();
        for (const QJsonValue& addonValue : addonsArray) {
            if (addonValue.isObject()) {
                QJsonObject addonObj = addonValue.toObject();
                Dependencies dependency;

                dependency.path = addonObj["path"].toString();
                dependency.addOnVersion = addonObj["addOnVersion"].toString();
                dependency.apiVersion = addonObj["apiVersion"].toString();
                dependency.library = addonObj.contains("library") ? addonObj["library"].toBool() : false;

                if (addonObj.contains("optionalDependencies") && addonObj["optionalDependencies"].isArray()) {
                    QJsonArray optDepArray = addonObj["optionalDependencies"].toArray();
                    for (const QJsonValue& depValue : optDepArray) {
                        dependency.optionalDependencies.append(depValue.toString());
                    }
                }

                if (addonObj.contains("requiredDependencies") && addonObj["requiredDependencies"].isArray()) {
                    QJsonArray reqDepArray = addonObj["requiredDependencies"].toArray();
                    for (const QJsonValue& depValue : reqDepArray) {
                        dependency.requiredDependencies.append(depValue.toString());
                    }
                }
                mod.addons.append(dependency);
            }
        }
    }

    QString lastUpdateStr = mod.lastUpdate;
    if (!lastUpdateStr.isEmpty()) {
        mod.lastUpdated = QDateTime::fromString(lastUpdateStr, Qt::ISODate);
        if (!mod.lastUpdated.isValid()) {
            bool isNumber = false;
            long long timestamp = lastUpdateStr.toLongLong(&isNumber);
            if (isNumber) {
                mod.lastUpdated = QDateTime::fromSecsSinceEpoch(timestamp);
            }
        }
    }

    return mod;
}

ModInfo Parser::fromCacheJson(const QJsonObject& json) {
    ModInfo mod;

    mod.id = json["id"].toString();
    mod.categoryId = json["categoryId"].toString();
    mod.version = json["version"].toString();
    mod.lastUpdate = json["lastUpdate"].toString();
    mod.title = json["title"].toString();
    mod.author = json["author"].toString();
    mod.fileInfoUri = json["fileInfoUri"].toString();
    mod.checksum = json["checksum"].toString();
    mod.library = json["library"].toBool();
    mod.donationUrl = QUrl(json["donationUrl"].toString());
    mod.downloadUrl = QUrl(json["downloadUrl"].toString());

    QString lastUpdateStr = json["lastUpdate"].toString();
    if (!lastUpdateStr.isEmpty()) {
        mod.lastUpdated = QDateTime::fromString(lastUpdateStr, Qt::ISODate);
        if (!mod.lastUpdated.isValid()) {
            mod.lastUpdated = QDateTime::fromString(lastUpdateStr, "yyyy-MM-dd");
        }
    }

    if (json.contains("gameVersions") && json["gameVersions"].isArray()) {
        QJsonArray versionsArray = json["gameVersions"].toArray();
        for (const QJsonValue& version : versionsArray) {
            mod.gameVersions.append(version.toString());
        }
    }

    if (json.contains("addons") && json["addons"].isArray()) {
        QJsonArray addonsArray = json["addons"].toArray();
        for (const QJsonValue& addonValue : addonsArray) {
            QJsonObject addonObj = addonValue.toObject();
            Dependencies dependency;
            dependency.path = addonObj["path"].toString();
            dependency.addOnVersion = addonObj["addOnVersion"].toString();
            dependency.apiVersion = addonObj["apiVersion"].toString();
            dependency.library = addonObj["library"].toBool();

            if (addonObj.contains("optionalDependencies") && addonObj["optionalDependencies"].isArray()) {
                QJsonArray optDepArray = addonObj["optionalDependencies"].toArray();
                for (const QJsonValue& depValue : optDepArray) {
                    dependency.optionalDependencies.append(depValue.toString());
                }
            }

            if (addonObj.contains("requiredDependencies") && addonObj["requiredDependencies"].isArray()) {
                QJsonArray reqDepArray = addonObj["requiredDependencies"].toArray();
                for (const QJsonValue& depValue : reqDepArray) {
                    dependency.requiredDependencies.append(depValue.toString());
                }
            }
            mod.addons.append(dependency);
        }
    }

    mod.isInstalled = json["isInstalled"].toBool(true);
    mod.installPath = json["installPath"].toString();

    return mod;
}

QJsonObject Parser::toCacheJson(const ModInfo& mod) {
    QJsonObject json;

    json["id"] = mod.id;
    json["categoryId"] = mod.categoryId;
    json["version"] = mod.version;
    json["lastUpdate"] = mod.lastUpdate;
    json["title"] = mod.title;
    json["author"] = mod.author;
    json["fileInfoUri"] = mod.fileInfoUri;
    json["downloads"] = mod.downloads;
    json["downloadsMonthly"] = mod.downloadsMonthly;
    json["checksum"] = mod.checksum;
    json["library"] = mod.library;
    json["donationUrl"] = mod.donationUrl.toString();
    json["downloadUrl"] = mod.downloadUrl.toString();
    json["gameVersions"] = QJsonArray::fromStringList(mod.gameVersions);

    QJsonArray addonsArray;
    for (const auto& dep : mod.addons) {
        QJsonObject depObj;
        depObj["path"] = dep.path;
        depObj["addOnVersion"] = dep.addOnVersion;
        depObj["apiVersion"] = dep.apiVersion;
        depObj["library"] = dep.library;
        depObj["optionalDependencies"] = QJsonArray::fromStringList(dep.optionalDependencies);
        depObj["requiredDependencies"] = QJsonArray::fromStringList(dep.requiredDependencies);
        addonsArray.append(depObj);
    }
    json["addons"] = addonsArray;

    json["isInstalled"] = mod.isInstalled;
    json["installPath"] = mod.installPath;

    return json;
}

QList<ModInfo> Parser::parseEsoMods(const QByteArray& jsonData) {
    QList<ModInfo> mods;
    QJsonParseError parseError;
    QJsonDocument document = QJsonDocument::fromJson(jsonData, &parseError);

    if (parseError.error != QJsonParseError::NoError || !document.isArray()) {
        // Caller should handle logging the error
        return mods;
    }

    QJsonArray modsJsonArray = document.array();
    for (const QJsonValue& value : modsJsonArray) {
        mods.append(fromEsoJson(value.toObject()));
    }
    return mods;
}