#pragma once

#include <QProgressDialog>
#include <QNetworkAccessManager>
#include <QUrl>

class HttpClient : public QObject {
    Q_OBJECT

public:
    explicit HttpClient(QObject* parent = nullptr);
    ~HttpClient();


};