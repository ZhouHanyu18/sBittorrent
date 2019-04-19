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

	pStatusForm = new StatusForm(this);
	pStatusForm->move(0, 370);
	//pStatusForm->show();

	qRegisterMetaType<AllTorrent>("AllTorrent&");
	connect(this, &MainWindow::thSignal, this, &MainWindow::onThSignal);
	boost::thread th(&MainWindow::setThread, this);
	th.detach();
}

void MainWindow::setThread()
{
	while(true)
	{
		Sess *p = Sess::getInstance();
		if (p->has_task)
		{
			Sleep(1000);
			AllTorrent items = p->getItem();
			emit thSignal(items);
			//pDownloadForm->setlist(items);
		}
	}
}

void MainWindow::onThSignal(AllTorrent& items)
{
	pDownloadForm->setList(items);
	pStatusForm->setStatus(items);
}

//void MainWindow::timerEvent(QTimerEvent *event)
//{
//	if (event->timerId() == m_nTimerID){
//	}
//}

MainWindow::~MainWindow()
{
	//killTimer(m_nTimerID);
    delete ui;
}

void MainWindow::resizeEvent(QResizeEvent *e)
{

	int realWidth = this->width();
	int realHeight = this->height();

	if (pDownloadForm != NULL)
	{
		pDownloadForm->resize(realWidth, realHeight-80);
		pDownloadForm->init();
	}

	if (pStatusForm != NULL)
	{
		pStatusForm->resize(realWidth, 30);
		pStatusForm->move(0, realHeight-30);
	}
}
//**************************Magnet和Torrent****************************************
void MainWindow::on_addMagnet_triggered()
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
		QMessageBox::information(this, "Error", QString::fromLocal8Bit("请输入正确链接"));
	}
	
}

void MainWindow::on_addTorrent_triggered()
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
//************************************绑定右键菜单******************************************************
void MainWindow::on_continue_2_triggered()
{
	pDownloadForm->getSelection();
	pDownloadForm->click_continueAction();
}

void MainWindow::on_stop_triggered()
{
	pDownloadForm->getSelection();
	pDownloadForm->click_stopAction();
}

void MainWindow::on_restart_triggered()
{
	pDownloadForm->getSelection();
	pDownloadForm->click_restartAction();
}

void MainWindow::on_allStop_triggered()
{
	pDownloadForm->stopAllTask();
}

void MainWindow::on_allContinue_triggered()
{
	pDownloadForm->continueAllTask();
}

void MainWindow::on_delete_all_triggered()
{
	pDownloadForm->deleteAllTask();
}

void MainWindow::on_delete_2_triggered()
{
	pDownloadForm->getSelection();
	pDownloadForm->click_deleteAction();
}
//****************************************************************************
void MainWindow::on_close_triggered()
{
	
}

void MainWindow::on_setting_triggered()
{

}

void MainWindow::on_about_triggered()
{

}

void MainWindow::on_update_triggered()
{

}

void MainWindow::on_search_triggered()
{

}

