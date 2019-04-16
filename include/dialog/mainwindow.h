#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "dialog/magnetdialog.h"
#include "dialog/torrentdialog.h"
#include "dialog/downloadform.h"

#include "test/debug_test.h"
#include "apps/download.h"
#include "apps/sess.h"
#include "tools/format.h"

#include <qfiledialog.h>
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
    void on_action_1_triggered();
	void resizeEvent(QResizeEvent *e);
    void on_action_Torrent_triggered();

private:
    Ui::MainWindow *ui;
	MagnetDialog *pMagnetDialog;
	TorrentDialog *pTorrentDialog;
	DownloadForm *pDownloadForm;
	int m_nTimerID;
public:
	virtual void timerEvent(QTimerEvent *event);	//定义定时器
};

#endif // MAINWINDOW_H
