#include "dialog/magnetDialog.h"
#include "ui_magnetDialog.h"
#include <qdebug.h>

MagnetDialog::MagnetDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::MagnetDialog)
{
    ui->setupUi(this);
}

MagnetDialog::~MagnetDialog()
{
    delete ui;
}

void MagnetDialog::on_buttonBox_clicked(QAbstractButton *button)
{
	if (ui->buttonBox->button(QDialogButtonBox::Ok) == (QPushButton*)button)	//判断按下的是否为"确定”按钮
	{
		sText = ui->textEdit->toPlainText();					//取得textEdit的输入内容
		//qDebug() << sText;
	}
}
