#ifndef DOWNLOADFORM_H
#define DOWNLOADFORM_H

#include <QWidget>
#include <QTableWidget>
#include <QProgressBar>
#include <QMenu>
#include <QDebug>
#include <QClipboard>
#include <QInputDialog>
#include <QDesktopServices>
#include <QUrl>
#include <QMessageBox>
#include <string>

//#include "dialog/menuform.h"
#include "apps/sess.h"

namespace Ui {
class DownloadForm;
}

class DownloadForm : public QWidget
{
    Q_OBJECT

public:
    explicit DownloadForm(QWidget *parent = 0);
    ~DownloadForm();

public:
	void init();
private:
    Ui::DownloadForm *ui;
	Sess *pSess = Sess::getInstance();
	QTableWidget *pTableWidget;
	QProgressBar *pProgressBar;			//进度条
	// 右键菜单
	QMenu *pMenu;
	QAction *continueAction;
	QAction *stopAction;
	QAction *playAction;
	QAction *restartAction;
	QAction *forceStartAction;
	QAction *deleteAction;
	QAction *copyAction;
	QAction *renameAction;
	QAction *openAction;
	bool bRandName;
	int aBool[100];
	QString RandName;
	//QModelIndexList selectedindexes;
	std::vector<int> rows;
	AllTorrent *list;
public:
	void addMagnet(const std::string str);
	void addTorrent(const std::string &str);
	bool has_task();
	void getItem();
	const AllTorrent& Items();
	void setList();
	void getSelection();
	void stopAllTask();
	void continueAllTask();
	void deleteAllTask();
	void saveResume();
public slots:
	void on_tableWidget_customContextMenuRequested(QPoint pos);		//右键响应
	void double_click(QTableWidgetItem *item);		//双击事件
	void click_continueAction();
	void click_stopAction();
	void click_forceStartAction();
	void click_playAction();
	void click_restartAction();
	void click_deleteAction();
	void click_copyAction();
	void click_renameAction();
	void click_openAction();
};

#endif // DOWNLOADFORM_H
