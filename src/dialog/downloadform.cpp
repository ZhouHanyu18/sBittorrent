#include "dialog/downloadform.h"
#include "ui_downloadform.h"

#include<QDesktopWidget>

DownloadForm::DownloadForm(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::DownloadForm)
{
    ui->setupUi(this);
	init();
	//�����Ҽ��˵�
	ui->tableWidget->setContextMenuPolicy(Qt::CustomContextMenu);

	pMenu = new QMenu(ui->tableWidget);
	continueAction = pMenu->addAction(QString::fromLocal8Bit("����"));
	stopAction = pMenu->addAction(QString::fromLocal8Bit("��ͣ"));
	playAction = pMenu->addAction(QString::fromLocal8Bit("����"));
	pMenu->addSeparator();
	restartAction = pMenu->addAction(QString::fromLocal8Bit("���¿�ʼ"));
	deleteAction = pMenu->addAction(QString::fromLocal8Bit("ɾ��"));
	pMenu->addSeparator();
	copyAction = pMenu->addAction(QString::fromLocal8Bit("�����ļ���"));
	renameAction = pMenu->addAction(QString::fromLocal8Bit("������"));
	openAction = pMenu->addAction(QString::fromLocal8Bit("�������ļ���"));

	connect(continueAction, &QAction::triggered, this, &DownloadForm::click_continueAction);
	connect(stopAction, &QAction::triggered, this, &DownloadForm::click_stopAction);
	connect(playAction, &QAction::triggered, this, &DownloadForm::click_playAction);
	connect(restartAction, &QAction::triggered, this, &DownloadForm::click_restartAction);
	connect(deleteAction, &QAction::triggered, this, &DownloadForm::click_deleteAction);
	connect(copyAction, &QAction::triggered, this, &DownloadForm::click_copyAction);
	connect(renameAction, &QAction::triggered, this, &DownloadForm::click_renameAction);
	connect(openAction, &QAction::triggered, this, &DownloadForm::click_openAction);
}

DownloadForm::~DownloadForm()
{
	delete pMenu;
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
	ui->tableWidget->setColumnWidth(2, width * 0.28);
	ui->tableWidget->setColumnWidth(3, width * 0.13);
	ui->tableWidget->setColumnWidth(4, width * 0.13);
	//ui->tableWidget->setColumnWidth(5, width * 0.13);
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
	ui->tableWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);	//��ѡ
	ui->tableWidget->setFrameShape(QFrame::NoFrame); //�����ޱ߿�
	ui->tableWidget->setShowGrid(false); //���ò���ʾ������
	//ui->tableWidget->setAlternatingRowColors(true);
	ui->tableWidget->verticalHeader()->setVisible(false);
	ui->tableWidget->horizontalHeader()->setStretchLastSection(true);

	//��������
	//ui->tableWidget->setRowCount(300);
	
}

void DownloadForm::setList(AllTorrent& list)
{
	for (int i = 0; i < list.size; ++i)
	{
		ui->tableWidget->setRowCount(list.size);
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
		ui->tableWidget->item(i, 0)->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);		//����
	}
	
}

void DownloadForm::on_tableWidget_customContextMenuRequested(QPoint pos)

{
	getSelection();
	pMenu->exec(QCursor::pos());
}

void DownloadForm::getSelection()

{
	std::vector<int> vecItemIndex;//����ѡ���е�����
	QItemSelectionModel *selections = ui->tableWidget->selectionModel(); //���ص�ǰ��ѡ��ģʽ  
	selectedindexes.clear();
	QSet< QPair<QModelIndex, int> > rowsSeen;

	const QItemSelection ranges = selections->selection();
	for (int i = 0; i < ranges.count(); ++i) {
		const QItemSelectionRange &range = ranges.at(i);
		QModelIndex parent = range.parent();
		for (int row = range.top(); row <= range.bottom(); row++) {
			QPair<QModelIndex, int> rowDef = qMakePair(parent, row);
			if (!rowsSeen.contains(rowDef)) {
				rowsSeen << rowDef;
				if (selections->isRowSelected(row, parent)) {
					qDebug() << row;
					selectedindexes.append(ui->tableWidget->model()->index(row, 0, parent));
				}
			}
		}
	}
	//���淽������ֱ���Qt bug
	//QModelIndexList selectedsList = selections->selectedIndexes(); //��������ѡ����ģ����Ŀ�����б�  
	//for (int i = 0; i < selectedsList.count(); i++)
	//{
	//	qDebug() << selectedsList.at(i).row();
	//	vecItemIndex.push_back(selectedsList.at(i).row());
	//}
	//selectedsList.clear();
	//std::sort(vecItemIndex.begin(), vecItemIndex.end());
	//vecItemIndex.erase(std::unique(vecItemIndex.begin(), vecItemIndex.end()), vecItemIndex.end());
}

void DownloadForm::click_continueAction()
{
	qDebug() << "click_continueAction";
}

void DownloadForm::click_stopAction()
{
	qDebug() << "click_stopAction";
}

void DownloadForm::click_playAction()
{
	qDebug() << "click_playAction";
}

void DownloadForm::click_restartAction()
{
	qDebug() << "click_restartAction";
}

void DownloadForm::click_deleteAction()
{
	qDebug() << "click_deleteAction";
}

void DownloadForm::click_copyAction()
{
	qDebug() << "click_copyAction";
}

void DownloadForm::click_renameAction()
{
	qDebug() << "click_renameAction";
}

void DownloadForm::click_openAction()
{
	qDebug() << "click_openAction";
}

void DownloadForm::stopAllTask()
{
	qDebug() << "stopAllTask";
}

void DownloadForm::continueAllTask()
{
	qDebug() << "continueAllTask";
}

void DownloadForm::deleteAllTask()
{
	qDebug() << "deleteAllTask";
}
