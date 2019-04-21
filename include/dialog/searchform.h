#ifndef SEARCHFORM_H
#define SEARCHFORM_H

#include <QWidget>

namespace Ui {
class SearchForm;
}

class SearchForm : public QWidget
{
    Q_OBJECT

public:
    explicit SearchForm(QWidget *parent = 0);
    ~SearchForm();

private:
    Ui::SearchForm *ui;
public:
	void init();
private slots:
    void on_SearchButton_clicked();
    void on_lineEdit_returnPressed();
};

#endif // SEARCHFORM_H
