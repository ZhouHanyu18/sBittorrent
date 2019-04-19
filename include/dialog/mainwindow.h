#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "dialog/magnetdialog.h"
#include "dialog/torrentdialog.h"
#include "dialog/downloadform.h"
#include "dialog/statusform.h"

#include "test/debug_test.h"
#include "apps/download.h"
#include "apps/information.h"
#include "tools/format.h"

#include <qfiledialog.h>
#include <QMetaType> 

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
	void on_addMagnet_triggered();
	void resizeEvent(QResizeEvent *e);
	void on_addTorrent_triggered();

private:
    Ui::MainWindow *ui;
	MagnetDialog *pMagnetDialog;
	TorrentDialog *pTorrentDialog;
	DownloadForm *pDownloadForm;
	StatusForm *pStatusForm;
	int m_nTimerID;
public:
	//virtual void timerEvent(QTimerEvent *event);	//定义定时器
	void setThread();
signals:
	void thSignal(AllTorrent& items);
private slots:
	void onThSignal(AllTorrent& items);
    void on_continue_2_triggered();
    void on_stop_triggered();
    void on_restart_triggered();
    void on_allStop_triggered();
    void on_allContinue_triggered();
    void on_delete_2_triggered();
    void on_close_triggered();
    void on_setting_triggered();
    void on_about_triggered();
    void on_update_triggered();
    void on_search_triggered();
    void on_delete_all_triggered();
};

#endif // MAINWINDOW_H
