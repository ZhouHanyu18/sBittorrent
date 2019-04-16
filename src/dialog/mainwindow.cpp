#include "dialog/mainwindow.h"
#include "ui_mainwindow.h"

//#pragma execution_character_set("utf-8")
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

	pDownloadForm = new DownloadForm(this);
	pDownloadForm->move(0, 50);
	pDownloadForm->show();
	m_nTimerID = this->startTimer(1000);
}

void MainWindow::timerEvent(QTimerEvent *event)
{
	if (event->timerId() == m_nTimerID){
		Sess *p = Sess::getInstance();
		if (p->has_task)
		{
			AllTorrent& items = p->getItem();
			pDownloadForm->setlist(items);
		}
		


	}
}

MainWindow::~MainWindow()
{
	killTimer(m_nTimerID);
    delete ui;
}

void MainWindow::resizeEvent(QResizeEvent *e)
{

	int realWidth = this->width();
	int realHeight = this->height();

	if (pDownloadForm != NULL)
	{
		pDownloadForm->resize(realWidth, realHeight - 100);
		pDownloadForm->init();
	}
}

void MainWindow::on_action_1_triggered()
{
	pMagnetDialog = new MagnetDialog(this);
	pMagnetDialog->exec();
	QString qStr = pMagnetDialog->getText();
	/*std::string sStr = std::string((const char *)qStr.toLocal8Bit());
	const char *cStr = sStr.c_str();*/

	QByteArray temp = qStr.toLocal8Bit();
	char *cStr = temp.data();

	if (strncmp(cStr, "magnet:", 7) == 0)
	{
		QMessageBox::information(this, "title", QString::fromLocal8Bit(cStr));
		Sess *p = Sess::getInstance();
	}
	
}

void MainWindow::on_action_Torrent_triggered()
{
	QString file_name = QFileDialog::getOpenFileName(NULL, QString::fromLocal8Bit("选择torrent文件"), ".", "*.torrent");
	QByteArray temp = file_name.toLocal8Bit();
	std::string str = temp.data();
	Sess *p = Sess::getInstance();
	p->addTorrent(tools::format::AsciiToUtf8(str));
	/*char *cStr = temp.data();
	char p[100][100];
	strcpy(p[0],  "");
	strcpy(p[1], cStr);
	Download download;
	download.main(2, p);
	qDebug() << cStr;*/
}
