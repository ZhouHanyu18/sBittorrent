#ifndef MEGNETDIALOG_H
#define MEGNETDIALOG_H

#include <QDialog>

namespace Ui {
class MegnetDialog;
}

class MegnetDialog : public QDialog
{
    Q_OBJECT

public:
    explicit MegnetDialog(QWidget *parent = 0);
    ~MegnetDialog();

private:
    Ui::MegnetDialog *ui;
};

#endif // MEGNETDIALOG_H
