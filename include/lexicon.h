#pragma once

#include <QObject>

class HttpClient;

class Lexicon : public QObject {
    Q_OBJECT
public:
    explicit Lexicon(QObject* parent = nullptr);
    ~Lexicon();

    Q_INVOKABLE void updateMasterList();
    void parseMasterList();
signals:
    void masterListReady(const QString& filePath);
    void downloadError(const QString& message);

private:
    HttpClient* m_httpClient;

    QString m_masterListPath;
};