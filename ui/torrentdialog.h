#ifndef TORRENTDIALOG_H
#define TORRENTDIALOG_H

#include <QDialog>

namespace Ui {
class TorrentDialog;
}

class TorrentDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TorrentDialog(QWidget *parent = 0);
    ~TorrentDialog();

private:
    Ui::TorrentDialog *ui;
};

#endif // TORRENTDIALOG_H
