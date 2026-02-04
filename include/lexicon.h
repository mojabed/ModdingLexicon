#pragma once

#include <QObject>
#include <QtQml>

class HttpClient;

class Lexicon : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_NAMED_ELEMENT(Lexicon)

public:
    explicit Lexicon(QObject* parent = nullptr);
    ~Lexicon();

signals:
    void masterListReady(const QString& filePath);
    void downloadError(const QString& message);

private:
    HttpClient* m_httpClient;

    void updateMasterList();
    void parseMasterList();

    QString m_masterListPath;
};