#pragma once

#include <QObject>

class HttpClient;

class LexiconQO : public QObject {
    Q_OBJECT
public:
    explicit LexiconQO(QObject* parent = nullptr);
    ~LexiconQO();

    Q_INVOKABLE void updateMasterList();
signals:
    void masterListReady(const QString& filePath);
    void downloadError(const QString& message);

private:
    HttpClient* m_httpClient;
};