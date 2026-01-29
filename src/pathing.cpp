#include "pathing.h"

#include <spdlog/spdlog.h>
#include <QStandardPaths>
#include <QFileInfo>
#include <QDir>

Pathing* Pathing::instance = nullptr;
QMutex Pathing::mtx;

Pathing::Pathing() {
    // Paths for Elder Scrolls Online
    docsPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    addonsPath = docsPath + "/Elder Scrolls Online/live/AddOns";
    // Paths for Modding Lexicon 
    appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    appConfigPath = appDataPath + "/config";

    spdlog::info("Pathing initialized:");
    spdlog::info("  Documents Path: {}", docsPath.toStdString());
    spdlog::info("  Addons Path: {}", addonsPath.toStdString());
    spdlog::info("  App Data Path: {}", appDataPath.toStdString());
    spdlog::info("  App Config Path: {}", appConfigPath.toStdString());
}

Pathing::~Pathing() {}

QString Pathing::getPaths() {
    return QString("Docs: %1\nAddons: %2\nAppData: %3\nAppConfig: %4")
        .arg(docsPath, addonsPath, appDataPath, appConfigPath);
}

bool Pathing::doesExist(const QString& path) {
    return QFileInfo::exists(path);
}

bool Pathing::isWritable(const QString& path) {
    QFileInfo info(path);
    if (info.exists()) {
        return info.isWritable();
    }
    // If it doesn't exist, check if its parent directory is writable
    return QDir(info.absolutePath()).exists() && QFileInfo(info.absolutePath()).isWritable();
}
