#include "dialog/mainwindow.h"
#include "ui_mainwindow.h"
#include "test/debug_test.h"

#include <qfiledialog.h>
//#pragma execution_character_set("utf-8")
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_action_triggered()
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
		//QMessageBox::information(this, "title", QString::fromLocal8Bit(cStr));

	}
}

void MainWindow::on_action_Torrent_triggered()
{
	QString file_name = QFileDialog::getOpenFileName(NULL, QString::fromLocal8Bit("选择torrent文件"), ".", "*.torrent");
	QByteArray temp = file_name.toLocal8Bit();
	char *cStr = temp.data();
	qDebug() << cStr;
}
