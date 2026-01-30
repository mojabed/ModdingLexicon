#include "pathing.h"

#include <spdlog/spdlog.h>
#include <QStandardPaths>
#include <QFileInfo>
#include <QDir>

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

QString Pathing::getPaths() {
    return QString("Docs: %1\nAddons: %2\nAppData: %3\nAppConfig: %4")
        .arg(m_paths.docs, m_paths.addons, m_paths.appData, m_paths.appConfig);
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
