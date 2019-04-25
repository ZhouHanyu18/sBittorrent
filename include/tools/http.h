#ifndef HTTP_H
#define HTTP_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QFile>
#include <QUrlQuery>
#include <QEventLoop>

class Http : public QObject
{
	Q_OBJECT
public:
	explicit Http(QObject *parent = nullptr);
	void Get(QUrl u, QString filename);

private:
	QNetworkAccessManager* pManager;
	QUrl url;
	QNetworkReply *reply;
	QString html_text;
	void setSSL(QNetworkRequest &request);
	~Http();
};

#endif // HTTP_H


