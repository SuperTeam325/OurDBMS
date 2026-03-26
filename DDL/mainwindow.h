#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "DDL.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_SubmitSQL_clicked();

private:
    Ui::MainWindow *ui;
    DDL::FieldType parseFieldType(const QString& typeStr);

};
#endif // MAINWINDOW_H
