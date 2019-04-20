#include "dialog/statusform.h"
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

void StatusForm::on_TotalDownloadRate_clicked()
{

}

void StatusForm::on_TotalUploadRate_clicked()
{

}

void StatusForm::setStatus(const AllTorrent& items)
{
	ui->TotalDownloadRate->setText(QString::fromLocal8Bit("%1 ( %2 )").
		arg(QString::fromLocal8Bit(items.total_download_rate.c_str())).
		arg(QString::fromLocal8Bit(items.total_download.c_str())));
	ui->TotalUploadRate->setText(QString::fromLocal8Bit("%1 ( %2 )").
		arg(QString::fromLocal8Bit(items.total_upload_rate.c_str())).
		arg(QString::fromLocal8Bit(items.total_upload.c_str())));
}