#include "dialog/torrentdialog.h"
#include "ui_torrentdialog.h"

TorrentDialog::TorrentDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::TorrentDialog)
{
    ui->setupUi(this);
}

TorrentDialog::~TorrentDialog()
{
    delete ui;
}

void TorrentDialog::on_buttonBox_clicked(QAbstractButton *button)
{

}
