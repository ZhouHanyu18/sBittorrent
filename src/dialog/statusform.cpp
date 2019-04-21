#include "dialog/statusform.h"
#include "dialog/mainwindow.h"
#include "ui_statusform.h"

StatusForm::StatusForm(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::StatusForm)
{
    ui->setupUi(this);
}

StatusForm::~StatusForm()
{
    delete ui;
}

void StatusForm::setStatus(const AllTorrent& items)
{
	ui->DownloadButton->setText(QString::fromLocal8Bit("È«²¿ ( %1 )").
		arg(QString::number(items.size)));
	ui->TotalDownloadRate->setText(QString::fromLocal8Bit("%1 ( %2 )").
		arg(QString::fromLocal8Bit(items.total_download_rate.c_str())).
		arg(QString::fromLocal8Bit(items.total_download.c_str())));
	ui->TotalUploadRate->setText(QString::fromLocal8Bit("%1 ( %2 )").
		arg(QString::fromLocal8Bit(items.total_upload_rate.c_str())).
		arg(QString::fromLocal8Bit(items.total_upload.c_str())));
}

void StatusForm::on_DownloadButton_clicked()
{
	MainWindow *p = (MainWindow *)parentWidget();
	p->pSearchForm->hide();
	p->pDownloadForm->show();
}

void StatusForm::on_TotalDownloadRate_clicked()
{

}

void StatusForm::on_TotalUploadRate_clicked()
{

}
