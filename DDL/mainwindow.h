#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "DDL.h"
#include "Lexer.h"
#include "Parser.h"
#include <QTreeWidgetItem>

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

    void on_SetPath_clicked();

private:
    Ui::MainWindow *ui;
    Parser p;
    DDL::DataBase db;
    DDL::Table t;
    DDL::FieldType parseFieldType(const QString& typeStr);
    //数据库文件路径
    QString DBpath;
    //结构显示
    void displayDB();
    //显示工具
    QStringList saveExpandedPaths(QTreeWidgetItem *item, const QString &parentPath);
    void restoreExpandedPaths(QTreeWidgetItem *item, const QString &parentPath, const QStringList &paths);
    //刷新显示
    void refreshDBTreeWithState();

};
#endif // MAINWINDOW_H
