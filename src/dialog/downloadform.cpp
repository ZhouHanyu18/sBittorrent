#include "dialog/downloadform.h"
#include "ui_downloadform.h"

#include<QDesktopWidget>

DownloadForm::DownloadForm(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::DownloadForm)
{
    ui->setupUi(this);
	init();
}

DownloadForm::~DownloadForm()
{
    delete ui;
}

void DownloadForm::init()
{
	//颜色
	//	QPalette palette(this->palette());
	//	palette.setColor(QPalette::Background, Qt::red);
	//	this->setPalette(palette);

	//设置tablewight	
	int width = this->width()-20;

	ui->tableWidget->setColumnCount(6);
	ui->tableWidget->setColumnWidth(0, width * 0.06);
	ui->tableWidget->setColumnWidth(1, width * 0.25);
	ui->tableWidget->setColumnWidth(2, width * 0.3);
	ui->tableWidget->setColumnWidth(3, width * 0.13);
	ui->tableWidget->setColumnWidth(4, width * 0.13);
	ui->tableWidget->setColumnWidth(5, width * 0.13);
	ui->tableWidget->verticalHeader()->setDefaultSectionSize(18);

	QStringList headText;
	headText << QString::fromLocal8Bit("编号")
		<< QString::fromLocal8Bit("文件名")
		<< QString::fromLocal8Bit("已完成")
		<< QString::fromLocal8Bit("状态")
		<< QString::fromLocal8Bit("下载速度")
		<< QString::fromLocal8Bit("上传速度");
	ui->tableWidget->setHorizontalHeaderLabels(headText);
	ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
	ui->tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
	ui->tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
	ui->tableWidget->setFrameShape(QFrame::NoFrame); //设置无边框
	ui->tableWidget->setShowGrid(false); //设置不显示格子线
	//ui->tableWidget->setAlternatingRowColors(true);
	ui->tableWidget->verticalHeader()->setVisible(false);
	ui->tableWidget->horizontalHeader()->setStretchLastSection(true);

	//设置行高
	ui->tableWidget->setRowCount(300);
	
}

void DownloadForm::setList(AllTorrent& list)
{
	for (int i = 0; i < list.size; ++i)
	{
		//设置进度条
		pProgressBar = new QProgressBar();//进度条初始化
		pProgressBar->setStyleSheet(
			"QProgressBar {border: 1px solid grey;   border-radius: 2px;"
			"background-color: #FFFFFF;"
			"text-align: center;}"
			"QProgressBar::chunk {background-color: rgb(0,250,0) ;}"
			);
		//pProgressBar->setRange(0, 0);
		pProgressBar->setValue(list.item[i].per);//设置进度条的初始值
		pProgressBar->setFormat(QString::fromLocal8Bit("%1/%2 %3%").
			arg(QString::fromLocal8Bit(list.item[i].download.c_str())).
			arg(QString::fromLocal8Bit(list.item[i].size.c_str())).
			arg(QString::number(list.item[i].per, 'f', 1)));


		QTableWidgetItem *itemID = new QTableWidgetItem(QString::number(list.item[i].queue_pos));
		QTableWidgetItem *itemName = new QTableWidgetItem(QString::fromLocal8Bit(list.item[i].name.c_str()));
		QTableWidgetItem *itemStatus = new QTableWidgetItem(QString::fromLocal8Bit(list.item[i].status.c_str()));
		QTableWidgetItem *itemDown = new QTableWidgetItem(QString::fromLocal8Bit(list.item[i].download_rate.c_str()));
		QTableWidgetItem *itemUp = new QTableWidgetItem(QString::fromLocal8Bit(list.item[i].upload_rate.c_str()));

		ui->tableWidget->setItem(i, 0, itemID);
		ui->tableWidget->setItem(i, 1, itemName);
		ui->tableWidget->setCellWidget(i, 2, pProgressBar);
		ui->tableWidget->setItem(i, 3, itemStatus);
		ui->tableWidget->setItem(i, 4, itemDown);
		ui->tableWidget->setItem(i, 5, itemUp);
	}
	
}
