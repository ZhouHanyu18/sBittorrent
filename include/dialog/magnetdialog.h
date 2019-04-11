#ifndef MAGNETDIALOG_H
#define MAGNETDIALOG_H

#include <QDialog>
#include <QAbstractButton>

namespace Ui {
class MagnetDialog;
}

class MagnetDialog : public QDialog
{
    Q_OBJECT

public:
    explicit MagnetDialog(QWidget *parent = 0);
    ~MagnetDialog();

private slots:
    void on_buttonBox_clicked(QAbstractButton *button);

private:
    Ui::MagnetDialog *ui;
	QString sText;

public:
	QString getText(){ return sText; }
};

#endif // MAGNETDIALOG_H
