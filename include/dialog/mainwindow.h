#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "dialog/magnetdialog.h"
#include "dialog/torrentdialog.h"
#include "dialog/downloadform.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_action_triggered();

    void on_action_Torrent_triggered();

private:
    Ui::MainWindow *ui;
	MagnetDialog *pMagnetDialog;
	TorrentDialog *pTorrentDialog;
	DownloadForm *pDownloadForm;
};

#endif // MAINWINDOW_H
