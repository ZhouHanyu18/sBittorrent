#ifndef DOWNLOADFORM_H
#define DOWNLOADFORM_H

#include <QWidget>
#include <QTableWidget>
#include <QProgressBar>

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
	QProgressBar *pProgressBar;			//½ø¶ÈÌõ
public:
	void setList(AllTorrent& list);
};

#endif // DOWNLOADFORM_H
