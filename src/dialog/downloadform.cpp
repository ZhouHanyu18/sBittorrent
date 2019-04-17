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
	//��ɫ
	//	QPalette palette(this->palette());
	//	palette.setColor(QPalette::Background, Qt::red);
	//	this->setPalette(palette);

	//����tablewight	
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
	headText << QString::fromLocal8Bit("���")
		<< QString::fromLocal8Bit("�ļ���")
		<< QString::fromLocal8Bit("�����")
		<< QString::fromLocal8Bit("״̬")
		<< QString::fromLocal8Bit("�����ٶ�")
		<< QString::fromLocal8Bit("�ϴ��ٶ�");
	ui->tableWidget->setHorizontalHeaderLabels(headText);
	ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
	ui->tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
	ui->tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
	ui->tableWidget->setFrameShape(QFrame::NoFrame); //�����ޱ߿�
	ui->tableWidget->setShowGrid(false); //���ò���ʾ������
	//ui->tableWidget->setAlternatingRowColors(true);
	ui->tableWidget->verticalHeader()->setVisible(false);
	ui->tableWidget->horizontalHeader()->setStretchLastSection(true);

	//�����и�
	ui->tableWidget->setRowCount(300);
	
}

void DownloadForm::setList(AllTorrent& list)
{
	for (int i = 0; i < list.size; ++i)
	{
		//���ý�����
		pProgressBar = new QProgressBar();//��������ʼ��
		pProgressBar->setStyleSheet(
			"QProgressBar {border: 1px solid grey;   border-radius: 2px;"
			"background-color: #FFFFFF;"
			"text-align: center;}"
			"QProgressBar::chunk {background-color: rgb(0,250,0) ;}"
			);
		//pProgressBar->setRange(0, 0);
		pProgressBar->setValue(list.item[i].per);//���ý������ĳ�ʼֵ
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
