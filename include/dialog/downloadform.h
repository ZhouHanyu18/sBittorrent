#ifndef DOWNLOADFORM_H
#define DOWNLOADFORM_H

#include <QWidget>
#include <QTableWidget>
#include <QProgressBar>
#include <QMenu>
#include <QDebug>

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
	QTableWidget *pTableWidget;
	QProgressBar *pProgressBar;			//进度条
	// 右键菜单
	QMenu *pMenu;
	QAction *continueAction;
	QAction *stopAction;
	QAction *playAction;
	QAction *restartAction;
	QAction *deleteAction;
	QAction *copyAction;
	QAction *renameAction;
	QAction *openAction;
	QModelIndexList selectedindexes;
public:
	void setList(AllTorrent& list);
	void getSelection();
	void stopAllTask();
	void continueAllTask();
	void deleteAllTask();
public slots:
	void on_tableWidget_customContextMenuRequested(QPoint pos);		//右键响应
	void click_continueAction();
	void click_stopAction();
	void click_playAction();
	void click_restartAction();
	void click_deleteAction();
	void click_copyAction();
	void click_renameAction();
	void click_openAction();
};

#endif // DOWNLOADFORM_H
