#pragma once

#include <QObject>

class HttpClient;

class Lexicon : public QObject {
    Q_OBJECT
public:
    explicit Lexicon(QObject* parent = nullptr);
    ~Lexicon();

    Q_INVOKABLE void updateMasterList();
signals:
    void masterListReady(const QString& filePath);
    void downloadError(const QString& message);

private:
    HttpClient* m_httpClient;
};