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
	//m_nTimerID = this->startTimer(100);
	boost::thread th(&MainWindow::setThread, this);
	th.detach();
}

void MainWindow::setThread()
{
	Sess *p = Sess::getInstance();
	if (p->has_task)
	{
		AllTorrent& items = p->getItem();
		pDownloadForm->setlist(items);
	}
}

void MainWindow::timerEvent(QTimerEvent *event)
{
	if (event->timerId() == m_nTimerID){
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

	QByteArray temp = qStr.toLocal8Bit();
	char *cStr = temp.data();

	if (strncmp(cStr, "magnet:", 7) == 0)
	{
		Sess *p = Sess::getInstance();
		p->addMagnet(tools::format::AsciiToUtf8(cStr));
	}
	else
	{
		QMessageBox::information(this, "Error", QString::fromLocal8Bit("链接不合法"));
	}
	
}

void MainWindow::on_action_Torrent_triggered()
{
	QString file_name = QFileDialog::getOpenFileName(NULL, QString::fromLocal8Bit("选择torrent文件"), ".", "*.torrent");
	if (file_name.length() > 0)
	{
		QByteArray temp = file_name.toLocal8Bit();
		std::string str = temp.data();
		Sess *p = Sess::getInstance();
		p->addTorrent(tools::format::AsciiToUtf8(str));
	}
	
}
