#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "DDL.h"
#include "Lexer.h"
#include "Parser.h"

class QDockWidget;
class QTabWidget;
class QLabel;
class DatabaseExplorer;
class SqlEditorPanel;
class ResultView;
class QueryHistoryPanel;

namespace DCL {
class DclFacade;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(DCL::DclFacade* facade, QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void executeSql(const QString& sql);
    void handleSetPath();
    void openTableSchema(const QString& dbName, const QString& tableName);
    void openTableData(const QString& dbName, const QString& tableName);
    void handleDeleteTable(const QString& dbName, const QString& tableName);
    void handleCreateTable(const QString& dbName);
    void handleModifyTable(const QString& dbName, const QString& tableName);
    void showSettings();
    void showAbout();
    void showExportPlaceholder();
    void updateStatusBar();
    void onHistoryQuerySelected(const QString& sql);

private:
    void createMenuBar();
    void createToolBar();
    void createStatusBar();
    void createDockWidgets();
    void createCentralWidget();
    void loadStyleSheet();

    DCL::DclFacade* m_dclFacade;
    Parser m_parser;
    DDL::DataBase m_db;
    DDL::Table m_table;
    QString m_dbPath;

    QTabWidget* m_centralTabs;
    DatabaseExplorer* m_dbExplorer;
    SqlEditorPanel* m_sqlPanel;
    QueryHistoryPanel* m_historyPanel;
    QLabel* m_statusLabel;

    QDockWidget* m_explorerDock;
    QDockWidget* m_historyDock;
    QDockWidget* m_sqlDock;
};

#endif // MAINWINDOW_H
