#ifndef STATUSFORM_H
#define STATUSFORM_H

#include <QWidget>

#include "apps/information.h"

namespace Ui {
class StatusForm;
}

class StatusForm : public QWidget
{
    Q_OBJECT

public:
    explicit StatusForm(QWidget *parent = 0);
    ~StatusForm();

private slots:
    void on_TotalDownloadRate_clicked();
	void on_TotalUploadRate_clicked();
private:
    Ui::StatusForm *ui;
	
public:
	void setStatus(AllTorrent& items);
};

#endif // STATUSFORM_H
