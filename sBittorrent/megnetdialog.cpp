#include "megnetdialog.h"
#include "ui_megnetdialog.h"

MegnetDialog::MegnetDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::MegnetDialog)
{
    ui->setupUi(this);
}

MegnetDialog::~MegnetDialog()
{
    delete ui;
}
