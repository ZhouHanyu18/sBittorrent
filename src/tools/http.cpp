#include "tools/http.h"

Http::Http(QObject *parent) : QObject(parent), reply(Q_NULLPTR), html_text("")
{
	pManager = new QNetworkAccessManager(this);
}

Http::~Http()
{
	if (reply != Q_NULLPTR)
	{
		reply->deleteLater();
	}
}

void Http::setSSL(QNetworkRequest &request)
{
	QSslConfiguration config;
	config = QSslConfiguration::defaultConfiguration();
	config.setPeerVerifyMode(QSslSocket::VerifyNone);
	config.setProtocol(QSsl::TlsV1_2);
	request.setSslConfiguration(config);
}
void Http::Get(QUrl u, QString filename)
{
	QEventLoop loop;
	QNetworkRequest request;
	url = u;

	setSSL(request);	//设置SSL,HTTPS协议需要SSL证书
	request.setUrl(url);

	if (reply != Q_NULLPTR) {//更改reply指向位置前一定要保证之前的已经delete
		reply->deleteLater();
	}
	qDebug() << u;
	qDebug() << pManager->supportedSchemes();
	reply = pManager->get(request);
	//qDebug() << "start get"<<url.url();
	connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
	loop.exec(); //开启事件循环，只有在循环完毕之后才会执行后面的语句。
	QByteArray bytes = reply->readAll();
	//qDebug() << bytes;
	const QVariant redirectionTarget = reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
	reply->deleteLater();
	reply = Q_NULLPTR;
	if (!redirectionTarget.isNull()) {//如果网址跳转重新请求
		const QUrl redirectedUrl = url.resolved(redirectionTarget.toUrl());
		//qDebug()<<"redirectedUrl:"<<redirectedUrl.url();
		Get(redirectedUrl, filename);
		return;
	}
	//qDebug()<<"finished";
	html_text = bytes;
	//qDebug()<<"get ready,read size:"<<html_text.size();
	QFile file(filename);//写出文件
	file.open(QFile::WriteOnly);
	file.write(bytes);
	file.close();
}
