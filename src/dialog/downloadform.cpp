#include "dialog/downloadform.h"
#include "ui_downloadform.h"

DownloadForm::DownloadForm(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::DownloadForm)
{
    ui->setupUi(this);
	//setWindowFlags(Qt::CustomizeWindowHint | Qt::FramelessWindowHint);
	//hide();
}

DownloadForm::~DownloadForm()
{
    delete ui;
}
