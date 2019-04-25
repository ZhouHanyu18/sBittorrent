#include "dialog/downloadform.h"
#include "ui_downloadform.h"

#include<QDesktopWidget>

DownloadForm::DownloadForm(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::DownloadForm)
{
    ui->setupUi(this);
	init();

	list = NULL;
	pSess = Sess::getInstance();

	//设置右键菜单
	ui->tableWidget->setContextMenuPolicy(Qt::CustomContextMenu);
	//设置双击事件
	connect(ui->tableWidget, &QTableWidget::itemDoubleClicked, this, &DownloadForm::double_click);
	//设置随机文件名
	bRandName = FALSE;
	memset(aBool, 0, sizeof(aBool));
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

void DownloadForm::addMagnet(const std::string str)
{
	if (pSess)
	{
		pSess->addMagnet(str);
	}
}

void DownloadForm::addTorrent(const std::string &str)
{
	if (pSess)
	{
		pSess->addTorrent(tools::format::AsciiToUtf8(str));
	}
}

bool DownloadForm::has_task()
{
	if (pSess)
	{
		return pSess->has_task;
	}
	return FALSE;
}

void DownloadForm::getItem()
{
	if (pSess)
	{
		list = &pSess->getItem();
	}
}

const AllTorrent& DownloadForm::Items()
{
	return *list;
}

void DownloadForm::setList()
{
	//ui->tableWidget->clearContents();
	if (list == nullptr)
		return;
	ui->tableWidget->setRowCount(list->size);
	for (int i = 0; i < list->size; ++i)
	{
		//设置进度条
		pProgressBar = new QProgressBar(this);//进度条初始化
		pProgressBar->setStyleSheet(
			"QProgressBar {border: 1px solid grey;   border-radius: 2px;"
			"background-color: #FFFFFF;"
			"text-align: center;}"
			"QProgressBar::chunk {background-color: rgb(0,250,0) ;}"
			);
		//pProgressBar->setRange(0, 0);
		pProgressBar->setValue(list->item[i].per);//设置进度条的初始值
		pProgressBar->setFormat(QString::fromLocal8Bit("%1/%2 %3%").
			arg(QString::fromLocal8Bit(list->item[i].download.c_str())).
			arg(QString::fromLocal8Bit(list->item[i].size.c_str())).
			arg(QString::number(list->item[i].per, 'f', 1)));


		QTableWidgetItem *itemID = new QTableWidgetItem(QString::number(list->item[i].queue_pos));
		QTableWidgetItem *itemName;
		if (aBool[i])
			itemName = new QTableWidgetItem(RandName + QString::number(i));
		else
			itemName = new QTableWidgetItem(QString::fromLocal8Bit(list->item[i].name.c_str()));
		
		QString &qStr = QString::fromLocal8Bit(list->item[i].status.c_str());
		QTableWidgetItem *itemStatus;
		if (qStr.endsWith("..."))
			itemStatus = new QTableWidgetItem(QIcon("media/icons/waitting.png"),
			qStr);
		else if (qStr.endsWith(".."))
			itemStatus = new QTableWidgetItem(QIcon("media/icons/download.png"),
			qStr);
		else if (qStr.endsWith("."))
			itemStatus = new QTableWidgetItem(QIcon("media/icons/stop.png"),
			qStr);
		else
			itemStatus = new QTableWidgetItem(QIcon("media/icons/ok.png"),
			qStr);
		
		QTableWidgetItem *itemDown = new QTableWidgetItem(QString::fromLocal8Bit(list->item[i].download_rate.c_str()));
		QTableWidgetItem *itemUp = new QTableWidgetItem(QString::fromLocal8Bit(list->item[i].upload_rate.c_str()));

		ui->tableWidget->setItem(i, 0, itemID);
		ui->tableWidget->setItem(i, 1, itemName);
		ui->tableWidget->setCellWidget(i, 2, pProgressBar);
		ui->tableWidget->setItem(i, 3, itemStatus);
		ui->tableWidget->setItem(i, 4, itemDown);
		ui->tableWidget->setItem(i, 5, itemUp);
		ui->tableWidget->item(i, 0)->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);		//居中
	}
	
}
//****************************************************************************************
void DownloadForm::double_click(QTableWidgetItem *item)
{
	qDebug() << "double_click";
	int i = item->row();
	rows.clear();
	rows.push_back(i);
	if (list->item[i].status.find("..") != -1)
		click_stopAction();
	else
		click_continueAction();
	
}

void DownloadForm::on_tableWidget_customContextMenuRequested(QPoint pos)
{
	pMenu = new QMenu(ui->tableWidget);
	continueAction = pMenu->addAction(QIcon("media/icons/start.png"), QString::fromLocal8Bit("继续"));
	stopAction = pMenu->addAction(QIcon("media/icons/stop.png"), QString::fromLocal8Bit("暂停"));
	forceStartAction = pMenu->addAction(QIcon("media/icons/force.png"), QString::fromLocal8Bit("强制继续"));
	playAction = pMenu->addAction(QIcon("media/icons/play.png"), QString::fromLocal8Bit("播放"));
	pMenu->addSeparator();
	restartAction = pMenu->addAction(QIcon("media/icons/reload.png"), QString::fromLocal8Bit("重新开始"));
	deleteAction = pMenu->addAction(QIcon("media/icons/delete.png"), QString::fromLocal8Bit("删除"));
	pMenu->addSeparator();
	copyAction = pMenu->addAction(QIcon("media/icons/Megnet.png"), QString::fromLocal8Bit("复制Magnet"));

	if (bRandName)
		renameAction = pMenu->addAction(QIcon("media/icons/restore.png"), QString::fromLocal8Bit("还原文件名"));
	else
		renameAction = pMenu->addAction(QIcon("media/icons/random.png"), QString::fromLocal8Bit("随机文件名"));

	openAction = pMenu->addAction(QIcon("media/icons/open.png"), QString::fromLocal8Bit("打开所在文件夹"));

	connect(continueAction, &QAction::triggered, this, &DownloadForm::click_continueAction);
	connect(stopAction, &QAction::triggered, this, &DownloadForm::click_stopAction);
	connect(forceStartAction, &QAction::triggered, this, &DownloadForm::click_forceStartAction);
	connect(playAction, &QAction::triggered, this, &DownloadForm::click_playAction);
	connect(restartAction, &QAction::triggered, this, &DownloadForm::click_restartAction);
	connect(deleteAction, &QAction::triggered, this, &DownloadForm::click_deleteAction);
	connect(copyAction, &QAction::triggered, this, &DownloadForm::click_copyAction);
	connect(renameAction, &QAction::triggered, this, &DownloadForm::click_renameAction);
	connect(openAction, &QAction::triggered, this, &DownloadForm::click_openAction);

	getSelection();
	pMenu->exec(QCursor::pos());
}

void DownloadForm::getSelection()

{
	std::vector<int> vecItemIndex;//保存选中行的索引
	QItemSelectionModel *selections = ui->tableWidget->selectionModel(); //返回当前的选择模式  
	//selectedindexes.clear();
	rows.clear();
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
					rows.push_back(row);
					//selectedindexes.append(ui->tableWidget->model()->index(row, 0, parent));
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
	if (rows.size() == 0)
		return;
	qDebug() << "click_continueAction";
	if (pSess)
	{
		pSess->continueDownload(rows);
	}
}

void DownloadForm::click_stopAction()
{
	if (rows.size() == 0)
		return;
	qDebug() << "click_stopAction";
	if (pSess)
	{
		pSess->stopDownload(rows);
	}
}

void DownloadForm::click_forceStartAction()
{
	if (rows.size() == 0)
		return;
	qDebug() << "click_forceStartAction";
	if (pSess)
	{
		pSess->forceStart(rows);
	}
}

void DownloadForm::click_playAction()
{
	if (rows.size() == 0)
		return;
	qDebug() << "click_playAction";
	click_openAction();
}

void DownloadForm::click_restartAction()
{
	if (rows.size() == 0)
		return;
	qDebug() << "click_restartAction";
	if (pSess)
	{
		pSess->restart(rows);
	}
}

void DownloadForm::click_deleteAction()
{
	if (rows.size() == 0)
		return;
	qDebug() << "click_deleteAction";
	switch (QMessageBox::information(this, QString::fromLocal8Bit("警告："),
		QString::fromLocal8Bit("是否删除已下载文件"),
		"&Yes", "&No", "&Cancel",
		0,      // Enter == button 0
		2)) { // Escape == button 2
	case 0: // No被点击或者Alt+S被按下或者Enter被按下。
		pSess->deleteTask(rows, TRUE);
		break;
	case 1: // Yes被点击或者Alt+D被按下。
		pSess->deleteTask(rows);
		break;
	case 2: // Cancel被点击或者Alt+C被按下或者Escape被按下。
		// 不退出
		break;
	}
}

void DownloadForm::click_copyAction()
{
	qDebug() << "click_copyAction";
	if (rows.size() == 0)
		return;
	string magnet = "";
	if (pSess)
	{
		pSess->getMagnet(rows, magnet);
	}
	qDebug() << magnet.c_str();
	QClipboard *clipboard = QApplication::clipboard();   //获取系统剪贴板指针
	clipboard->setText(QString::fromLocal8Bit(magnet.c_str()));

	//QString originalText = clipboard->text();	     //获取剪贴板上文本信息
	//clipboard->setText(newText);		             //设置剪贴板内容</span>

}

void DownloadForm::click_renameAction()
{
	qDebug() << "click_renameAction";
	bRandName = !bRandName;
	if (bRandName)
	{
		bool ok;
		RandName = QInputDialog::getText(this, QString::fromLocal8Bit("随机重命名"),
			QString::fromLocal8Bit("请输入："), QLineEdit::Normal,
			QString::fromLocal8Bit("葫芦娃"), &ok);
		if (ok)
		{
			for each(auto i in rows)
			{
				aBool[i] = 1;
			}
			if (rows.size() == 0)
			{
				if (list)
					for (int i = 0; i < list->size; ++i)
					{
						aBool[i] = 1;
					}
			}
		}
		else
		{
			bRandName = !bRandName;
		}
	}
	else
	{
		memset(aBool, 0, sizeof(aBool));
	}
	
	
	qDebug() << RandName;
	/*
	p->rename(rows, "zhy");*/
}

void DownloadForm::click_openAction()
{
	qDebug() << "click_openAction";
	
	string url= "";
	if (pSess)
	{
		pSess->getFileUrl(rows, url);
	}
	QDesktopServices::openUrl(QUrl::fromLocalFile(url.c_str()));
	//QDesktopServices::openUrl(QUrl(QString::fromLocal8Bit(url.c_str()), QUrl::TolerantMode));
}

void DownloadForm::stopAllTask()
{
	qDebug() << "stopAllTask";
	if (pSess)
	{
		pSess->stopAll();
	}
}

void DownloadForm::continueAllTask()
{
	qDebug() << "continueAllTask";
	if (pSess)
	{
		pSess->continueAll();
	}
}

void DownloadForm::deleteAllTask()
{
	qDebug() << "deleteAllTask";
	switch (QMessageBox::information(this, QString::fromLocal8Bit("警告："),
		QString::fromLocal8Bit("是否删除已下载文件"),
		"&Yes", "&No", "&Cancel",
		0,      // Enter == button 0
		2)) { // Escape == button 2
	case 0: // No被点击或者Alt+S被按下或者Enter被按下。
		pSess->deleteAll(TRUE);
		break;
	case 1: // Yes被点击或者Alt+D被按下。
		pSess->deleteAll();
		break;
	case 2: // Cancel被点击或者Alt+C被按下或者Escape被按下。
		// 不退出
		break;
	}
}

void DownloadForm::saveResume()
{
	if (pSess)
	{
		pSess->saveResume();
	}
}