#ifndef DOWNLOADFORM_H
#define DOWNLOADFORM_H

#include <QWidget>

namespace Ui {
class DownloadForm;
}

class DownloadForm : public QWidget
{
    Q_OBJECT

public:
    explicit DownloadForm(QWidget *parent = 0);
    ~DownloadForm();

private:
    Ui::DownloadForm *ui;
};

#endif // DOWNLOADFORM_H
