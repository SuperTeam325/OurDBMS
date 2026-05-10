#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include "DDL.h"
#include "Lexer.h"
#include "Parser.h"
#include "user_repository.h"
#include "permission_service.h"
#include <QTreeWidgetItem>
#include <QListWidgetItem>

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
    ~MainWindow();

private slots:
    // SQL 执行
    void on_SubmitSQL_clicked();
    // 设置数据库路径
    void on_btnSetPath_clicked();
    // 导航切换
    void on_btnDataMgmt_clicked();
    void on_btnConsole_clicked();
    void on_btnLogViewer_clicked();
    // 数据管理工具栏
    void on_btnRefreshTree_clicked();
    // 日志查看
    void on_btnRefreshLogs_clicked();
    void onLogFileItemClicked(QListWidgetItem *item);
    void onLogSearchChanged(const QString& keyword);
    // 树形控件右键菜单
    void onTreeRightClicked(const QPoint &pos);
    void refreshTree();
    void createTableMenu();
    void deleteTableMenu();
    void modifyTableMenu();
    void viewTableMenu();
    void viewTableDataMenu();
    void deleteDatabase();

private:
    Ui::MainWindow *ui;
    Parser p;
    DDL::DataBase db;
    DDL::Table t;
    DDL::FieldType parseFieldType(const QString& typeStr);
    // DCL
    DCL::UserRepository userReposity;
    DCL::PermissionService userPermission;
    DCL::DclFacade* dclFacade;

    // 数据库文件路径
    QString DBpath;
    // 结构显示
    void displayDB();
    // 系统数据库显示
    void displaySDB();
    // 显示工具
    QStringList saveExpandedPaths(QTreeWidgetItem *item, const QString &parentPath);
    void restoreExpandedPaths(QTreeWidgetItem *item, const QString &parentPath, const QStringList &paths);
    // 刷新显示
    void refreshDBTreeWithState();
    // 导航按钮互斥
    void setNavButtonChecked(QPushButton* active);
    // 日志工具
    void loadLogFileList();
    void loadLogContent(const QString& date);
    QHash<QString, QList<QPair<QString, QString>>> m_dateLogs; // date → [(dbName, filePath)]
};

#endif // MAINWINDOW_H
