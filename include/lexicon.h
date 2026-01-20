#pragma once

class Lexicon {
public:
    Lexicon(const Lexicon& obj) = delete;
    Lexicon& operator=(const Lexicon&) = delete;

    static Lexicon* getInstance() {
        if (instance == nullptr) {
            QMutexLocker locker(&mtx);
            if (instance == nullptr) {
                instance = new Lexicon();
            }
        }
        return instance;
    }

private:
    static Lexicon* instance;
    static QMutex mtx;

    Lexicon() {}
};