#include "pathing.h"

#include <spdlog/spdlog.h>
#include <QStandardPaths>

Pathing* Pathing::instance = nullptr;
QMutex Pathing::mtx;

Pathing::Pathing() {
    // Paths for Elder Scrolls Online
    m_paths.docs = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    m_paths.addons = m_paths.docs + "/Elder Scrolls Online/live/AddOns";
    // Paths for Modding Lexicon 
    m_paths.appData = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    m_paths.appConfig = m_paths.appData + "/config";

    spdlog::info("Pathing initialized:");
    spdlog::info("  Documents Path: {}", m_paths.docs.toStdString());
    spdlog::info("  Addons Path: {}", m_paths.addons.toStdString());
    spdlog::info("  App Data Path: {}", m_paths.appData.toStdString());
    spdlog::info("  App Config Path: {}", m_paths.appConfig.toStdString());
}

Pathing::~Pathing() {}
