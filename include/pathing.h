#pragma once

#include <QMutex>
#include <QMutexLocker>
#include <QString>

enum class PathType {
    Docs,
    Addons,
    AppData,
    AppConfig
};

struct PathCollection {
    QString docs;
    QString addons;
    QString appData;
    QString appConfig;

    QString operator[](PathType type) const {
        switch (type) {
        case PathType::Docs: return docs;
        case PathType::Addons: return addons;
        case PathType::AppData: return appData;
        case PathType::AppConfig: return appConfig;
        default: return QString();
        }
    }
};

class Pathing {
public:
    Pathing(const Pathing& obj) = delete;
    Pathing& operator=(const Pathing&) = delete;

    static Pathing* getInstance() {
        if (instance == nullptr) {
            QMutexLocker locker(&mtx);
            if (instance == nullptr) {
                instance = new Pathing();
            }
        }
        return instance;
    }

    const PathCollection& paths() const { return m_paths; }

    void setAddonsPath(const QString& path);

private:
    static Pathing* instance;
    static QMutex mtx;

    PathCollection m_paths;

    Pathing();
    ~Pathing();
};