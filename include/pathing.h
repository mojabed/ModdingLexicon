#pragma once

#include <QMutex>
#include <QMutexLocker>
#include <QString>

// Singleton class for managing user paths for ESO and for the Lexicon app
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

    QString getPaths();

private:
    static Pathing* instance;
    static QMutex mtx;

    QString docsPath, addonsPath, appConfigPath, appDataPath;

    bool doesExist(const QString& path);
    bool isWritable(const QString& path);

    Pathing();
    ~Pathing();
};