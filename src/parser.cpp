#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

#include "parser.h"

const QString Parser::m_downloadUrlPrefix = "https://www.esoui.com/downloads/download";

namespace {
QString readStringValue(const QJsonObject& object, const QStringList& keys) {
    for (const QString& key : keys) {
        if (object.contains(key) && object[key].isString()) {
            const QString value = object[key].toString();
            if (!value.isEmpty()) {
                return value;
            }
        }
    }
    return {};
}

QString readCategoryId(const QJsonObject& object) {
    QStringList candidates{"id", "categoryId", "category_id", "category"};
    QString value = readStringValue(object, candidates);
    if (!value.isEmpty()) {
        return value;
    }

    for (const QString& key : candidates) {
        if (object.contains(key) && object[key].isDouble()) {
            return QString::number(object[key].toDouble());
        }
    }

    return {};
}

QString readCategoryName(const QJsonObject& object) {
    QStringList candidates{"name", "categoryName", "category_name", "title", "label"};
    return readStringValue(object, candidates);
}

QString readCategoryIconUrl(const QJsonObject& object) {
    QStringList candidates{"iconUrl", "icon_url", "icon", "imageUrl", "image_url", "thumbnailUrl", "thumbnail_url"};
    return readStringValue(object, candidates);
}
}

ModInfo Parser::fromEsoJson(const QJsonObject& json) {
    ModInfo mod;
    mod.isInstalled = false;

    mod.id = QString::number(json["id"].toDouble());
    mod.categoryId = json["categoryId"].isString() ? json["categoryId"].toString() : QString::number(json["categoryId"].toDouble());
    mod.categoryName = json.contains("categoryName") && json["categoryName"].isString()
        ? json["categoryName"].toString()
        : QString();
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
    mod.categoryName = json.contains("categoryName") ? json["categoryName"].toString() : QString();
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
    json["categoryName"] = mod.categoryName;
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
        return mods;
    }

    QJsonArray modsJsonArray = document.array();
    for (int i = 1; i < modsJsonArray.size(); ++i) {
        mods.append(fromEsoJson(modsJsonArray[i].toObject()));
    }
    return mods;
}

QMap<QString, QString> Parser::parseCategoryNames(const QByteArray& jsonData) {
    QMap<QString, QString> categories;
    QJsonParseError parseError;
    QJsonDocument document = QJsonDocument::fromJson(jsonData, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        return categories;
    }

    auto addCategory = [&](const QJsonObject& object) {
        const QString id = readCategoryId(object);
        const QString name = readCategoryName(object);
        if (!id.isEmpty() && !name.isEmpty()) {
            categories[id] = name;
        }
    };

    if (document.isArray()) {
        for (const QJsonValue& value : document.array()) {
            if (value.isObject()) {
                addCategory(value.toObject());
            }
        }
    } else if (document.isObject()) {
        const QJsonObject root = document.object();
        QList<QString> candidateArrays{"categories", "data", "items", "results"};
        for (const QString& key : candidateArrays) {
            if (root.contains(key) && root[key].isArray()) {
                for (const QJsonValue& value : root[key].toArray()) {
                    if (value.isObject()) {
                        addCategory(value.toObject());
                    }
                }
                break;
            }
        }

        if (categories.isEmpty()) {
            addCategory(root);
        }
    }

    return categories;
}

QMap<QString, QString> Parser::parseCategoryIcons(const QByteArray& jsonData) {
    QMap<QString, QString> icons;
    QJsonParseError parseError;
    QJsonDocument document = QJsonDocument::fromJson(jsonData, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        return icons;
    }

    auto addCategory = [&](const QJsonObject& object) {
        const QString id = readCategoryId(object);
        const QString iconUrl = readCategoryIconUrl(object);
        if (!id.isEmpty() && !iconUrl.isEmpty()) {
            icons[id] = iconUrl;
        }
    };

    if (document.isArray()) {
        for (const QJsonValue& value : document.array()) {
            if (value.isObject()) {
                addCategory(value.toObject());
            }
        }
    } else if (document.isObject()) {
        const QJsonObject root = document.object();
        QList<QString> candidateArrays{"categories", "data", "items", "results"};
        for (const QString& key : candidateArrays) {
            if (root.contains(key) && root[key].isArray()) {
                for (const QJsonValue& value : root[key].toArray()) {
                    if (value.isObject()) {
                        addCategory(value.toObject());
                    }
                }
                break;
            }
        }

        if (icons.isEmpty()) {
            addCategory(root);
        }
    }

    return icons;
}