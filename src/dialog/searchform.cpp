#include "dialog/searchform.h"
#include "ui_searchform.h"

SearchForm::SearchForm(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SearchForm)
{
    ui->setupUi(this);
	ui->lineEdit->addAction(QIcon("media/icons/search.png"),QLineEdit::LeadingPosition);
	init();

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
	ui->tableWidget->setColumnWidth(1, width * 0.25);
	ui->tableWidget->setColumnWidth(2, width * 0.28);
	ui->tableWidget->setColumnWidth(3, width * 0.13);
	ui->tableWidget->setColumnWidth(4, width * 0.13);
	//ui->tableWidget->setColumnWidth(5, width * 0.13);
	ui->tableWidget->verticalHeader()->setDefaultSectionSize(18);

	QStringList headText;
	headText << QString::fromLocal8Bit("编号")
		<< QString::fromLocal8Bit("文件名")
		<< QString::fromLocal8Bit("xxx")
		<< QString::fromLocal8Bit("xxx")
		<< QString::fromLocal8Bit("xxx")
		<< QString::fromLocal8Bit("xxx");
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
void SearchForm::on_SearchButton_clicked()
{

}

void SearchForm::on_lineEdit_returnPressed()
{

}
