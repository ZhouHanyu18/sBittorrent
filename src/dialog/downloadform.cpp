#include "dialog/downloadform.h"
#include "ui_downloadform.h"

#include<QDesktopWidget>

DownloadForm::DownloadForm(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::DownloadForm)
{
    ui->setupUi(this);
	init();
	//设置右键菜单
	ui->tableWidget->setContextMenuPolicy(Qt::CustomContextMenu);

	pMenu = new QMenu(ui->tableWidget);
	continueAction = pMenu->addAction(QString::fromLocal8Bit("继续"));
	stopAction = pMenu->addAction(QString::fromLocal8Bit("暂停"));
	playAction = pMenu->addAction(QString::fromLocal8Bit("播放"));
	pMenu->addSeparator();
	restartAction = pMenu->addAction(QString::fromLocal8Bit("重新开始"));
	deleteAction = pMenu->addAction(QString::fromLocal8Bit("删除"));
	pMenu->addSeparator();
	copyAction = pMenu->addAction(QString::fromLocal8Bit("复制文件名"));
	renameAction = pMenu->addAction(QString::fromLocal8Bit("重命名"));
	openAction = pMenu->addAction(QString::fromLocal8Bit("打开所在文件夹"));

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
	//颜色
	//	QPalette palette(this->palette());
	//	palette.setColor(QPalette::Background, Qt::red);
	//	this->setPalette(palette);

	//设置tablewight	
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
	headText << QString::fromLocal8Bit("编号")
		<< QString::fromLocal8Bit("文件名")
		<< QString::fromLocal8Bit("已完成")
		<< QString::fromLocal8Bit("状态")
		<< QString::fromLocal8Bit("下载速度")
		<< QString::fromLocal8Bit("上传速度");
	ui->tableWidget->setHorizontalHeaderLabels(headText);
	ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
	ui->tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
	ui->tableWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);	//多选
	ui->tableWidget->setFrameShape(QFrame::NoFrame); //设置无边框
	ui->tableWidget->setShowGrid(false); //设置不显示格子线
	//ui->tableWidget->setAlternatingRowColors(true);
	ui->tableWidget->verticalHeader()->setVisible(false);
	ui->tableWidget->horizontalHeader()->setStretchLastSection(true);

	//设置行数
	//ui->tableWidget->setRowCount(300);
	
}

void DownloadForm::setList(AllTorrent& list)
{
	for (int i = 0; i < list.size; ++i)
	{
		ui->tableWidget->setRowCount(list.size);
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
		ui->tableWidget->item(i, 0)->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);		//居中
	}
	
}

void DownloadForm::on_tableWidget_customContextMenuRequested(QPoint pos)

{
	getSelection();
	pMenu->exec(QCursor::pos());
}

void DownloadForm::getSelection()

{
	std::vector<int> vecItemIndex;//保存选中行的索引
	QItemSelectionModel *selections = ui->tableWidget->selectionModel(); //返回当前的选择模式  
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
	//下面方法会出现崩溃Qt bug
	//QModelIndexList selectedsList = selections->selectedIndexes(); //返回所有选定的模型项目索引列表  
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
