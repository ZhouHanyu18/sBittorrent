#ifndef SEARCHFORM_H
#define SEARCHFORM_H

#include <QWidget>
#include <QTableWidget>
#include <QMenu>
#include <QAction>
#include <QClipboard>

#include "tools/http.h"

namespace Ui {
class SearchForm;
}

struct item
{
	QString name;
	QString size;
	QString magnet;
	QString time;
	QString Seeders;
	QString Leechers;
};

class SearchForm : public QWidget
{
    Q_OBJECT

public:
    explicit SearchForm(QWidget *parent = 0);
    ~SearchForm();

private:
    Ui::SearchForm *ui;
	QStringList search(QString regularexp, QString data);
	QVector<item> list;
	std::vector<int> rows;
	void getSelection();
	// 右键菜单
	QMenu *pMenu;
	QAction *downloadAction;
	QAction *copyAction;
public:
	void init();
	void setList();
	static bool bCmp2;
	static bool bDefault;
private slots:
    void on_SearchButton_clicked();
    void on_lineEdit_returnPressed();
	void record_sortbyclounm(int clounm);
	void double_click(QTableWidgetItem *item);		// 双击下载事件
	void on_tableWidget_customContextMenuRequested(QPoint pos);		// 右键响应
	// 右键菜单响应
	void click_downloadAction();
	void click_copyAction();

};

#endif // SEARCHFORM_H
