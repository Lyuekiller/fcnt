#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
namespace Ui {
class Widget;
}

class Widget : public QWidget
{
    Q_OBJECT

public:
    explicit Widget(QWidget *parent = nullptr);
    ~Widget();

private slots:
    void on_openButton_clicked();

    void on_readButton_clicked();

    void on_targetButton_clicked();

    void on_openFcntButton_clicked();

    void on_cutButton_clicked();

    void on_targetSegyButton_clicked();

    void on_fileButton_clicked();

    void on_readButton_2_clicked();

    void on_openButton_2_clicked();

    void on_mergeButton_clicked();

    void on_openButton_3_clicked();

    void on_quButton_clicked();

    void on_openButton_4_clicked();

    void on_tarButton_clicked();

    void on_composeButton_clicked();

    void on_oButton_clicked();

    void on_testButton_clicked();

private:
    Ui::Widget *ui;
};

#endif // WIDGET_H
