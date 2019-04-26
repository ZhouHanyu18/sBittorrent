#include "dialog/mainwindow.h"
#include "ui_mainwindow.h"

//#pragma execution_character_set("utf-8")
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
	QWidget *spacer = new QWidget(this);
	spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	//toolBar is a pointer to an existing toolbar  
	ui->mainToolBar->addWidget(spacer);
	ui->mainToolBar->addAction(ui->praise);
	pDownloadForm = new DownloadForm(this);
	pDownloadForm->move(0, 50);
	pDownloadForm->show();
	//m_nTimerID = this->startTimer(100);

	pStatusForm = new StatusForm(this);
	pStatusForm->move(0, 370);
	pStatusForm->show();

	pSearchForm = new SearchForm(this);
	pSearchForm->move(0, 50);
	pSearchForm->hide();
	//qRegisterMetaType<AllTorrent>("AllTorrent&");
	connect(this, &MainWindow::thSignal, this, &MainWindow::onThSignal);
	bThread = TRUE;
	th = boost::thread(&MainWindow::setThread, this);
	th.detach();
}

void MainWindow::setThread()
{
	while (bThread)
	{
		if (pDownloadForm->has_task())
		{
			Sleep(1000);
			pDownloadForm->getItem();
			emit thSignal();
		}
	}
}

void MainWindow::onThSignal()
{
	pDownloadForm->setList();
	pStatusForm->setStatus(pDownloadForm->Items());
}

//void MainWindow::timerEvent(QTimerEvent *event)
//{
//	if (event->timerId() == m_nTimerID){
//	}
//}

// 重写closeEvent: 确认退出对话框
void MainWindow::closeEvent(QCloseEvent *event)
{
	on_save_triggered();
	bThread = FALSE;
	th.~thread();
	this->hide();
	while (!pDownloadForm->quit());
	event->accept(); // 接受退出信号，程序退出
}


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

	if (pSearchForm != NULL)
	{
		pSearchForm->resize(realWidth, realHeight - 80);
		pSearchForm->init();
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
		pDownloadForm->addMagnet(cStr);
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
		pDownloadForm->addTorrent(str);
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
void MainWindow::on_save_triggered()
{
	pDownloadForm->saveResume();
}

void MainWindow::on_search_triggered()
{
	pDownloadForm->hide();
	pSearchForm->show();

}

void MainWindow::on_setting_triggered()
{
	pSettingDialog = new SettingDialog(this);
	pSettingDialog->exec();
}

void MainWindow::on_about_triggered()
{
	pAboutDialog = new AboutDialog(this);
	pAboutDialog->exec();
	
}

void MainWindow::on_update_triggered()
{

}

void MainWindow::on_praise_triggered()
{
	QDesktopServices::openUrl(QUrl(QString("https://github.com/ZhouHanyu18/sBittorrent"), QUrl::TolerantMode));
}
