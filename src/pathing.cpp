#include "pathing.h"

#include <QStandardPaths>
#include <QFileInfo>
#include <QDir>

Pathing* Pathing::paths = nullptr;
QMutex Pathing::mutex;

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