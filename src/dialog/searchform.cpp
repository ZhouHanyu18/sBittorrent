#include "dialog/searchform.h"
#include "dialog/mainwindow.h"
#include "ui_searchform.h"
#include <math.h>
#include <Thread>

SearchForm::SearchForm(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SearchForm)
{
    ui->setupUi(this);
	ui->lineEdit->addAction(QIcon("media/icons/search.png"),QLineEdit::LeadingPosition);
	init();

	//设置右键菜单
	ui->tableWidget->setContextMenuPolicy(Qt::CustomContextMenu);
	//重写排序
	connect(ui->tableWidget->horizontalHeader(), SIGNAL(sectionClicked(int)), this, SLOT(record_sortbyclounm(int)));
	//设置双击事件
	connect(ui->tableWidget, &QTableWidget::itemDoubleClicked, this, &SearchForm::double_click);
}

SearchForm::~SearchForm()
{
    delete ui;
}

void SearchForm::init()
{
	int width = this->width() - 20;

	ui->tableWidget->setColumnCount(6);
	ui->tableWidget->setColumnWidth(0, width * 0.06);
	ui->tableWidget->setColumnWidth(1, width * 0.50);
	ui->tableWidget->setColumnWidth(2, width * 0.12);
	ui->tableWidget->setColumnWidth(3, width * 0.12);
	ui->tableWidget->setColumnWidth(4, width * 0.1);
	//ui->tableWidget->setColumnWidth(5, width * 0.13);
	ui->tableWidget->verticalHeader()->setDefaultSectionSize(18);

	QStringList headText;
	headText << QString::fromLocal8Bit("编号")
		<< QString::fromLocal8Bit("文件名")
		<< QString::fromLocal8Bit("大小")
		<< QString::fromLocal8Bit("日期")
		<< QString::fromLocal8Bit("做种")
		<< QString::fromLocal8Bit("吸血");
	ui->tableWidget->setHorizontalHeaderLabels(headText);
	ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
	ui->tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
	ui->tableWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);	//多选
	ui->tableWidget->setFrameShape(QFrame::NoFrame); //设置无边框
	ui->tableWidget->setShowGrid(false); //设置不显示格子线
	//ui->tableWidget->setAlternatingRowColors(true);
	ui->tableWidget->verticalHeader()->setVisible(false);
	ui->tableWidget->horizontalHeader()->setStretchLastSection(true);
}

void tran(QString str, float &size)
{
	QString temp = str.toUpper();
	QVector<QString>suffix = { "KB", "MB", "GB", "TB", "PB" };
	int i = 0;
	for (; i < suffix.size(); i++)
	{
		if (temp.indexOf(suffix.at(i)) != -1)
		{
			break;
		}
	}
	temp.remove(suffix[i]);
	size = temp.toFloat() * pow(1024, i);
}

bool cmp(item const lItem, item const rItem)
{
	float lSize;
	float rSize;
	tran(lItem.size, lSize);
	tran(rItem.size, rSize);
	if (SearchForm::bCmp2)
		return lSize>rSize;
	else
		return lSize<rSize;

}

bool SearchForm::bCmp2 = true;
bool SearchForm::bDefault = true;
void SearchForm::record_sortbyclounm(int clounm)
{
	if (clounm >= ui->tableWidget->columnCount() || clounm < 0)
		return;
	switch (clounm)
	{
	case 2:
		qSort(list.begin(), list.end(), &cmp);
		setList();
		SearchForm::bCmp2 = !SearchForm::bCmp2;
		break;
	case 0:
	case 1:
	case 3:
	case 4:
	case 5:
	default:
		if (SearchForm::bDefault)
			ui->tableWidget->sortItems(clounm, Qt::AscendingOrder);
		else
			ui->tableWidget->sortItems(clounm, Qt::DescendingOrder);
		SearchForm::bDefault = !SearchForm::bDefault;
		break;
	}
}

void SearchForm::setList()
{
	//ui->tableWidget->clearContents();
	ui->tableWidget->setRowCount(list.size());
	for (int i = 0; i < list.size(); ++i)
	{
		QTableWidgetItem* num = new QTableWidgetItem;
		num->setData(Qt::DisplayRole, i);
		ui->tableWidget->setItem(i, 0, num);
		ui->tableWidget->setItem(i, 1, new QTableWidgetItem(list[i].name));
		ui->tableWidget->setItem(i, 2, new QTableWidgetItem(list[i].size));
		ui->tableWidget->setItem(i, 3, new QTableWidgetItem(list[i].time));

		QTableWidgetItem* Seeders = new QTableWidgetItem;
		Seeders->setData(Qt::DisplayRole, list[i].Seeders.toFloat());
		ui->tableWidget->setItem(i, 4, Seeders);
		//ui->tableWidget->setItem(i, 4, new QTableWidgetItem(list[i].Seeders));

		QTableWidgetItem* Leechers = new QTableWidgetItem;
		Leechers->setData(Qt::DisplayRole, list[i].Leechers.toFloat());
		ui->tableWidget->setItem(i, 5, Leechers);
		//ui->tableWidget->setItem(i, 5, new QTableWidgetItem(list[i].Leechers));

		ui->tableWidget->item(i, 0)->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);		//居中
	}

}


QStringList SearchForm::search(QString regularexp, QString data){
	QStringList searchdata;
	QRegularExpression regularExpression(regularexp);
	int index = 0;
	QRegularExpressionMatch match;
	do {
		match = regularExpression.match(data, index);
		if (match.hasMatch()) {
			index = match.capturedEnd();
			searchdata.append(match.captured(0));
		}
		else{
			break;
		}
	} while (index < data.length());
	//qDebug()<<searchdata;
	if (searchdata.size() == 0)
		searchdata.append(0);
	return searchdata;
}

void SearchForm::TheadDownload(int n)
{
}

int SearchForm::regZooqle()
{
	QFile file(".resume/search");
	if (!file.open(QIODevice::ReadWrite | QIODevice::Text)) {
		qDebug() << "Can't open the file!" << endl;
		return 0;
	}
	QTextStream stream(&file);
	stream.setCodec("utf-8");
	QString data = stream.readAll();
	file.close();
	QStringList page_num = search("(?<=>)[0-9]+(?=</a></li><li><a[^>]*aria-label=\"Next\")", data);
	QStringList source = search("(small\" href=\"/).*?(magnet:[?]xt=urn:btih:).*?(?=</div></div></td></tr>)", data);

	for (int i = 0; i < source.size(); ++i)
	{
		item temp;
		temp.name = (search("(?<=small\" href=\"/)[^>]*(?=.html\">)", source[i]).at(0));
		temp.size = search("(?<=>)[0-9., ]+(?i)(b|kb|mb|gb|tb|pb)(?=</div>)", source[i]).at(0);
		temp.time = search("(?<=text-nowrap text-muted smaller\">).*?(?=</td><td)", source[i]).at(0);
		temp.Seeders = search("(?<=Seeders:)[0-9 ]+", source[i]).at(0);
		temp.Leechers = search("(?<=Leechers:)[0-9 ]+", source[i]).at(0);
		temp.magnet = search("(magnet:[?]xt=urn:btih:).*?(?=\"><i)", source[i]).at(0);
		list.append(temp);
	}
	setList();
	if (page_num.size() > 0)
		return page_num[0].toInt();
	else
		return 0;
}

int SearchForm::regBtAnt()
{
	QFile file(".resume/search");
	if (!file.open(QIODevice::ReadWrite | QIODevice::Text)) {
		qDebug() << "Can't open the file!" << endl;
		return 0;
	}
	QTextStream stream(&file);
	stream.setCodec("utf-8");
	QString data = stream.readAll().replace('\n', "");;
	file.close();

	QStringList page_num = search("(?<=-first-asc-)[0-9]+(?=\">)", data);
	QStringList mLists = search("(item-list).*?(magnet:[?]xt=urn:btih:).*?(?=class)", data);
	for (int i = 0; i < mLists.size(); ++i)
	{
		item temp;
		QStringList name = search("(?<=<p>).*?(?=<span>[0-9., ]+)", mLists[i]);
		temp.name = name[0].replace(QRegularExpression("<.*?>"), "");
		temp.size = search("(?<=<span>)[0-9., ]+(?i)(b|kb|mb|gb|tb|pb)(?=</span>)", mLists[i]).at(0);
		temp.time = search("(?<=blue-pill\">).*?(?=</b>)", mLists[i]).at(1);
		temp.Seeders = search("(?<=yellow-pill\">)[0-9]+", mLists[i]).at(0);
		temp.Leechers = temp.Seeders;
		temp.magnet = search("(magnet:[?]xt=urn:btih:).*?(?=\")", mLists[i]).at(0);
		list.append(temp);
	}
	setList();
	if (page_num.size() > 0)
		return page_num.back().toInt();
	else
		return 0;
}

void SearchForm::on_SearchButton_clicked()
{
	list.clear();
	html = new Http(this);

	QUrl url = QUrl(QString("https://zooqle.com/search"));
	QUrlQuery query;
	query.addQueryItem("q", ui->lineEdit->text());
	query.addQueryItem("pg", QString::fromLocal8Bit("1"));
	url.setQuery(query);
	html->Get(url, QString(".resume/search"));

	int nPage = regZooqle();

	if (nPage > 1)
	{
		for (int i = 1; i < nPage; ++i)
		{
			query.removeQueryItem("pg");
			query.addQueryItem("pg", QString::number(i+1));
			url.setQuery(query);
			html->Get(url, QString(".resume/search"));
			regZooqle();
		}
	}
	QString web = "http://www.btaot.com/search/";
	QString text = ui->lineEdit->text();
	QString page = "-first-asc-";

	url = QUrl(web + text + page + "1");
	qDebug() << url;
	html->Get(url, QString(".resume/search"));
	nPage = regBtAnt();

	if (nPage > 1)
	{
		for (int i = 1; i < nPage; ++i)
		{
			QUrl url = QUrl(web + text + page + QString::number(i + 1));
			qDebug() << url;
			html->Get(url, QString(".resume/search"));
			regBtAnt();
		}
	}
}

void SearchForm::on_lineEdit_returnPressed()
{
	on_SearchButton_clicked();
}

void SearchForm::double_click(QTableWidgetItem *item)
{
	int row = item->row();
	int i = ui->tableWidget->item(row, 0)->text().toInt();
	rows.clear();
	rows.push_back(i);
	qDebug() << rows[0];
	click_downloadAction();
}

void SearchForm::on_tableWidget_customContextMenuRequested(QPoint pos)
{
	pMenu = new QMenu(ui->tableWidget);
	downloadAction = pMenu->addAction(QIcon("media/icons/start.png"), QString::fromLocal8Bit("下载"));
	pMenu->addSeparator();
	copyAction = pMenu->addAction(QIcon("media/icons/Megnet.png"), QString::fromLocal8Bit("复制Magnet"));

	connect(downloadAction, &QAction::triggered, this, &SearchForm::click_downloadAction);
	connect(copyAction, &QAction::triggered, this, &SearchForm::click_copyAction);

	getSelection();
	pMenu->exec(QCursor::pos());
}

void SearchForm::getSelection()
{
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
}

void SearchForm::click_downloadAction()
{
	MainWindow *p = (MainWindow *)parentWidget();
	for each (auto i in rows)
	{
		qDebug() << i;
		QByteArray temp = list[i].magnet.toLocal8Bit();
		char *cStr = temp.data();
		p->pDownloadForm->addMagnet(cStr);
	}
}

void SearchForm::click_copyAction()
{
	qDebug() << "click_copyAction";
	if (rows.size() == 0)
		return;
	QString magnet = "";
	for each (auto i in rows)
	{
		magnet.append(list[i].magnet);
		magnet.append('\n');
	}
	qDebug() << magnet;

	QClipboard *clipboard = QApplication::clipboard();   //获取系统剪贴板指针
	clipboard->setText(magnet);
}