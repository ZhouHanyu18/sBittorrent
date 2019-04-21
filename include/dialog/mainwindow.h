#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QCloseEvent>

#include "dialog/magnetdialog.h"
#include "dialog/torrentdialog.h"
#include "dialog/downloadform.h"
#include "dialog/statusform.h"
#include "dialog/searchform.h"
#include "dialog/settingdialog.h"
#include "dialog/aboutdialog.h"

#include "test/debug_test.h"
#include "apps/download.h"

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

public:
    Ui::MainWindow *ui;
	void closeEvent(QCloseEvent *event);    // 重写closeEvent的申明
	MagnetDialog *pMagnetDialog;
	TorrentDialog *pTorrentDialog;
	DownloadForm *pDownloadForm;
	StatusForm *pStatusForm;
	SearchForm *pSearchForm;
	AboutDialog *pAboutDialog;
	SettingDialog *pSettingDialog;
	int m_nTimerID;
	//virtual void timerEvent(QTimerEvent *event);	//定义定时器
	boost::thread th;
	void setThread();

signals:
	void thSignal();
private slots:
	void onThSignal();
    void on_continue_2_triggered();
    void on_stop_triggered();
    void on_restart_triggered();
    void on_allStop_triggered();
    void on_allContinue_triggered();
    void on_delete_2_triggered();
	void on_save_triggered();
    void on_setting_triggered();
    void on_about_triggered();
    void on_update_triggered();
    void on_search_triggered();
    void on_delete_all_triggered();
    void on_praise_triggered();
};

#endif // MAINWINDOW_H
