#pragma once

#include <QMutex>
#include <QMutexLocker>
#include <QString>

class Pathing {
public:
    Pathing(const Pathing& obj) = delete;
    Pathing& operator=(const Pathing&) = delete;

    static Pathing* getInstance() {
        if (paths == nullptr) {
            QMutexLocker locker(&mtx);
            if (paths == nullptr) {
                paths = new Pathing();
            }
        }
        return paths;
    }

    QString getPaths();

private:
    static Pathing* paths;
    static QMutex mtx;

    QString docsPath, addonsPath, appConfigPath, appDataPath;

    bool doesExist(const QString& path);
    bool isWritable(const QString& path);

    Pathing() {}
};