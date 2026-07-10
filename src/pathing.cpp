#include "pathing.h"

#include <spdlog/spdlog.h>
#include <QStandardPaths>
#include <QSettings>
#include <QDir>

Pathing* Pathing::instance = nullptr;
QMutex Pathing::mtx;

Pathing::Pathing() {
    // Paths for Elder Scrolls Online
    m_paths.docs = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    m_paths.addons = m_paths.docs + "/Elder Scrolls Online/live/AddOns";

    m_paths.appData = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    m_paths.appConfig = m_paths.appData + "/config";

    QSettings settings(QStringLiteral("ModdingLexicon"), QStringLiteral("ModdingLexicon"));
    const QString customAddons = settings.value(QStringLiteral("addonsPath")).toString();
    if (!customAddons.isEmpty() && QDir(customAddons).exists()) {
        m_paths.addons = customAddons;
    }

    spdlog::info("Pathing initialized:");
    spdlog::info("  Documents Path: {}", m_paths.docs.toStdString());
    spdlog::info("  Addons Path: {}", m_paths.addons.toStdString());
    spdlog::info("  App Data Path: {}", m_paths.appData.toStdString());
    spdlog::info("  App Config Path: {}", m_paths.appConfig.toStdString());
}

Pathing::~Pathing() {}

void Pathing::setAddonsPath(const QString& path) {
    m_paths.addons = path;
    QSettings settings(QStringLiteral("ModdingLexicon"), QStringLiteral("ModdingLexicon"));
    settings.setValue(QStringLiteral("addonsPath"), path);
    spdlog::info("Addons path changed to: {}", path.toStdString());
}
