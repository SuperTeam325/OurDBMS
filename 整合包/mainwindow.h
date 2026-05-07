#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "DDL.h"
#include "Lexer.h"
#include "Parser.h"
#include "user_repository.h"
#include "permission_service.h"
#include <QTreeWidgetItem>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}

namespace DCL {
class DclFacade;
}

QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(DCL::DclFacade* facade, QWidget *parent = nullptr);

    //MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:

    void on_SubmitSQL_clicked();

    void on_SetPath_clicked();

    void onTreeRightClicked(const QPoint &pos);    // 右键菜单
    void refreshTree();                             // 刷新
    void createTableMenu();                         // 新建表
    void deleteTableMenu();                         // 删除表
    void modifyTableMenu();                         // 修改表
    void viewTableMenu();                           //查看表
    void viewTableDataMenu();                       //查看数据
    void deleteDatabase();                          //删除数据库


private:

    Ui::MainWindow *ui;
    Parser p;
    DDL::DataBase db;
    DDL::Table t;
    DDL::FieldType parseFieldType(const QString& typeStr);
    //DCL
    DCL::UserRepository userReposity;
    DCL::PermissionService userPermission;
    DCL::DclFacade* dclFacade;

    //数据库文件路径
    QString DBpath;
    //结构显示
    void displayDB();
    //系统数据库显示
    void displaySDB();
    //显示工具
    QStringList saveExpandedPaths(QTreeWidgetItem *item, const QString &parentPath);
    void restoreExpandedPaths(QTreeWidgetItem *item, const QString &parentPath, const QStringList &paths);
    //刷新显示
    void refreshDBTreeWithState();

};
#endif // MAINWINDOW_H
