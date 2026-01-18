#pragma once

struct Dependencies {
    QString path;
    QString addOnVersion;
    QString apiVersion;
    bool library = false;
    QStringList optionalDependencies;
    QStringList requiredDependencies;
};

struct ModInfo {
    // Identification
    QString id;
    QString categoryId;
    QString title;
    QString author;

    // Version information
    QString version;
    QString lastUpdate;
    QDateTime lastUpdated;

    // Metadata
    QString fileInfoUri;
    QStringList gameVersions;
    QString checksum;
    QList<Dependencies> addons;
    bool library = false;
    QUrl donationUrl;

    // Installation status
    bool isInstalled = false;
    QString installPath;
    qint64 sizeInBytes = 0;

    // Download information
    QUrl downloadUrl;
    int downloads = 0;
    int downloadsMonthly = 0;
    int favorites = 0;
    bool hasUpdate = false;

    QString getFormattedDate() const {
        return lastUpdated.isValid() ? lastUpdated.toString("dd-MM-yyyy") : "";
    }
};