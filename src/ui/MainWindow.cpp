#include "src/ui/MainWindow.h"
#include "src/ui/DatabaseExplorer.h"
#include "src/ui/SqlEditorPanel.h"
#include "src/ui/ResultView.h"
#include "src/ui/QueryHistoryPanel.h"
#include "src/ui/SettingsDialog.h"
#include "src/ui/AboutDialog.h"
#include "dialog.h"
#include "ui_dialog.h"
#include "Log.h"
#include "Lexer.h"
#include "DML.h"
#include "DCL/dcl_facade.h"
#include "DCL/session_manager.h"

#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QDockWidget>
#include <QTabWidget>
#include <QLabel>
#include <QAction>
#include <QMessageBox>
#include <QFile>
#include <QDir>
#include <QApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QDebug>

// ---- Constructor / Destructor ----

MainWindow::MainWindow(DCL::DclFacade* facade, QWidget *parent)
    : QMainWindow(parent)
    , m_dclFacade(facade)
{
    loadStyleSheet();

    if (m_dclFacade && m_dclFacade->isLoggedIn()) {
        setWindowTitle("Mini DBMS - " + m_dclFacade->currentSession().username);
    } else {
        setWindowTitle("Mini DBMS - 未登录");
    }

    createStatusBar();
    createDockWidgets();
    createCentralWidget();
    createMenuBar();
    createToolBar();

    resize(1400, 900);

    // Wire SqlEditorPanel signals
    connect(m_sqlPanel, &SqlEditorPanel::sqlSubmitted,
            this, &MainWindow::executeSql);
    connect(m_sqlPanel, &SqlEditorPanel::pathChangeRequested,
            this, &MainWindow::handleSetPath);

    // Wire DatabaseExplorer signals
    connect(m_dbExplorer, &DatabaseExplorer::viewTableRequested,
            this, &MainWindow::openTableSchema);
    connect(m_dbExplorer, &DatabaseExplorer::viewDataRequested,
            this, &MainWindow::openTableData);
    connect(m_dbExplorer, &DatabaseExplorer::deleteTableRequested,
            this, &MainWindow::handleDeleteTable);
    connect(m_dbExplorer, &DatabaseExplorer::createTableRequested,
            this, &MainWindow::handleCreateTable);
    connect(m_dbExplorer, &DatabaseExplorer::modifyTableRequested,
            this, &MainWindow::handleModifyTable);

    // Wire QueryHistoryPanel signal
    connect(m_historyPanel, &QueryHistoryPanel::querySelected,
            this, &MainWindow::onHistoryQuerySelected);

    updateStatusBar();
}

MainWindow::~MainWindow()
{
}

// ---- Style ----

void MainWindow::loadStyleSheet()
{
    QFile styleFile(":/style/app");
    if (styleFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        setStyleSheet(styleFile.readAll());
        styleFile.close();
    }
}

// ---- Menu Bar ----

void MainWindow::createMenuBar()
{
    QMenuBar* mb = menuBar();

    // File menu
    QMenu* fileMenu = mb->addMenu(QString::fromUtf8("文件(&F)"));
    fileMenu->addAction(QIcon(":/icons/settings"),
                        QString::fromUtf8("设置(&S)..."),
                        this, &MainWindow::showSettings);
    fileMenu->addSeparator();
    fileMenu->addAction(QString::fromUtf8("退出(&X)"),
                        this, &QMainWindow::close, QKeySequence::Quit);

    // Edit menu
    QMenu* editMenu = mb->addMenu(QString::fromUtf8("编辑(&E)"));
    QAction* clearAction = editMenu->addAction(QIcon(":/icons/clear"),
                                                QString::fromUtf8("清空输出(&C)"));
    connect(clearAction, &QAction::triggered, m_sqlPanel, &SqlEditorPanel::clearOutput);

    // View menu
    QMenu* viewMenu = mb->addMenu(QString::fromUtf8("查看(&V)"));
    viewMenu->addAction(m_explorerDock->toggleViewAction());
    viewMenu->addAction(m_historyDock->toggleViewAction());
    viewMenu->addAction(m_sqlDock->toggleViewAction());
    viewMenu->addSeparator();
    QAction* refreshAction = viewMenu->addAction(QIcon(":/icons/refresh"),
                                                  QString::fromUtf8("刷新数据库树(&R)"));
    refreshAction->setShortcut(QKeySequence("F5"));
    connect(refreshAction, &QAction::triggered, m_dbExplorer, &DatabaseExplorer::refreshTree);

    // Tools menu
    QMenu* toolsMenu = mb->addMenu(QString::fromUtf8("工具(&T)"));
    QAction* exportAction = toolsMenu->addAction(QIcon(":/icons/export"),
                                                  QString::fromUtf8("导出结果(&E)..."));
    connect(exportAction, &QAction::triggered, this, &MainWindow::showExportPlaceholder);

    // Help menu
    QMenu* helpMenu = mb->addMenu(QString::fromUtf8("帮助(&H)"));
    helpMenu->addAction(QString::fromUtf8("关于(&A)..."), this, &MainWindow::showAbout);
}

// ---- Tool Bar ----

void MainWindow::createToolBar()
{
    QToolBar* tb = addToolBar(QString::fromUtf8("主工具栏"));
    tb->setMovable(false);
    tb->setIconSize(QSize(20, 20));

    QAction* runAction = tb->addAction(QIcon(":/icons/run"),
                                        QString::fromUtf8("执行SQL"));
    connect(runAction, &QAction::triggered, m_sqlPanel, &SqlEditorPanel::submitSql);

    tb->addSeparator();

    QAction* refreshAction = tb->addAction(QIcon(":/icons/refresh"),
                                            QString::fromUtf8("刷新"));
    connect(refreshAction, &QAction::triggered, m_dbExplorer, &DatabaseExplorer::refreshTree);

    tb->addSeparator();

    QAction* settingsAction = tb->addAction(QIcon(":/icons/settings"),
                                             QString::fromUtf8("设置"));
    connect(settingsAction, &QAction::triggered, this, &MainWindow::showSettings);
}

// ---- Status Bar ----

void MainWindow::createStatusBar()
{
    m_statusLabel = new QLabel;
    statusBar()->addPermanentWidget(m_statusLabel, 1);
}

void MainWindow::updateStatusBar()
{
    QString user = (m_dclFacade && m_dclFacade->isLoggedIn())
        ? m_dclFacade->currentSession().username
        : QString::fromUtf8("未登录");
    QString db = m_db.name.isEmpty() ? QString::fromUtf8("无") : m_db.name;
    m_statusLabel->setText(QString::fromUtf8("用户: %1  |  数据库: %2  |  Mini DBMS v0.1").arg(user, db));
}

// ---- Dock Widgets ----

void MainWindow::createDockWidgets()
{
    // Left: Database Explorer
    m_explorerDock = new QDockWidget(QString::fromUtf8("数据库浏览器"), this);
    m_explorerDock->setObjectName("ExplorerDock");
    m_dbExplorer = new DatabaseExplorer(m_dclFacade, PROJECT_ROOT_DIR, this);
    m_explorerDock->setWidget(m_dbExplorer);
    addDockWidget(Qt::LeftDockWidgetArea, m_explorerDock);

    // Right: Query History
    m_historyDock = new QDockWidget(QString::fromUtf8("查询历史"), this);
    m_historyDock->setObjectName("HistoryDock");
    m_historyPanel = new QueryHistoryPanel(this);
    m_historyDock->setWidget(m_historyPanel);
    addDockWidget(Qt::RightDockWidgetArea, m_historyDock);

    // Bottom: SQL Editor
    m_sqlDock = new QDockWidget(QString::fromUtf8("SQL编辑器"), this);
    m_sqlDock->setObjectName("SqlDock");
    m_sqlPanel = new SqlEditorPanel(this);
    m_sqlDock->setWidget(m_sqlPanel);
    addDockWidget(Qt::BottomDockWidgetArea, m_sqlDock);
}

// ---- Central Widget ----

void MainWindow::createCentralWidget()
{
    m_centralTabs = new QTabWidget(this);
    m_centralTabs->setTabsClosable(true);
    m_centralTabs->setDocumentMode(true);
    setCentralWidget(m_centralTabs);

    connect(m_centralTabs, &QTabWidget::tabCloseRequested, this, [this](int index) {
        if (m_centralTabs->count() <= 1) return;  // keep at least one tab
        QWidget* w = m_centralTabs->widget(index);
        m_centralTabs->removeTab(index);
        w->deleteLater();
    });
}

// ---- SQL Execution ----

void MainWindow::executeSql(const QString& sql)
{
    if (sql.isEmpty()) return;

    // DCL gate
    if (m_dclFacade) {
        QString dclMessage;
        QString dclError;
        if (m_dclFacade->tryHandleSessionSql(sql, dclMessage, dclError)) {
            if (!dclError.isEmpty()) {
                m_sqlPanel->appendOutput("SQL执行失败：" + dclError);
            } else {
                m_sqlPanel->appendOutput(dclMessage);
                if (m_dclFacade->isLoggedIn()) {
                    setWindowTitle("Mini DBMS - " + m_dclFacade->currentSession().username);
                } else {
                    setWindowTitle("Mini DBMS - 未登录");
                }
            }
            m_sqlPanel->clearInput();
            m_dbExplorer->refreshTree();
            m_historyPanel->addQuery(sql);
            updateStatusBar();
            return;
        }
    }

    // Login check
    if (!m_dclFacade || !m_dclFacade->isLoggedIn()) {
        m_sqlPanel->appendOutput("SQL执行失败：未登录");
        return;
    }

    // Permission check
    QString authError;
    if (!m_dclFacade->authorizeSql(sql, authError)) {
        m_sqlPanel->appendOutput("SQL执行失败：" + authError);
        return;
    }

    // DDL: CREATE DATABASE
    if (sql.startsWith("CREATE DATABASE", Qt::CaseInsensitive)) {
        try {
            m_parser.paraseCreateDB(sql, m_dbPath);
            m_sqlPanel->appendOutput("数据库创建成功");
            m_dbExplorer->refreshTree();
            m_sqlPanel->clearInput();
        } catch (const std::invalid_argument& e) {
            m_sqlPanel->appendOutput("SQL执行失败：" + QString(e.what()));
        }
    }
    // DDL: DROP DATABASE
    else if (sql.startsWith("DROP DATABASE", Qt::CaseInsensitive)) {
        try {
            QRegularExpression re("drop\\s+database\\s+([a-zA-Z_][a-zA-Z0-9_]*)",
                                  QRegularExpression::CaseInsensitiveOption);
            QRegularExpressionMatch m = re.match(sql);
            if (!m.hasMatch()) throw std::invalid_argument("语法错误：DROP DATABASE");
            QString dbName = m.captured(1);
            QFile file("db_config.json");
            QJsonObject obj;
            if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
                file.close();
                if (!doc.isNull()) obj = doc.object();
            }
            if (!obj.contains(dbName)) throw std::invalid_argument("数据库不存在");
            QString dbPath = obj[dbName].toString();
            obj.remove(dbName);
            if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                file.write(QJsonDocument(obj).toJson());
                file.close();
            }
            QDir dir(dbPath);
            if (dir.exists()) dir.removeRecursively();
            if (m_db.name == dbName) { m_db.name.clear(); m_db.path.clear(); }
            m_sqlPanel->appendOutput("数据库删除成功：" + dbName);
            m_dbExplorer->refreshTree();
            m_sqlPanel->clearInput();
        } catch (const std::invalid_argument& e) {
            m_sqlPanel->appendOutput("SQL执行失败：" + QString(e.what()));
        }
    }
    // DDL: USE
    else if (sql.startsWith("USE", Qt::CaseInsensitive)) {
        try {
            m_parser.paraseUSEDB(sql, m_db);
            m_dclFacade->setCurrentDatabase(m_db.name);
            m_sqlPanel->appendOutput(QString("切换成功,当前数据库：%1").arg(m_db.name));
            Log::writeToLog(m_db.path, m_dclFacade->currentSession().username, sql);
            m_sqlPanel->clearInput();
            updateStatusBar();
        } catch (const std::invalid_argument& e) {
            m_sqlPanel->appendOutput("SQL执行失败：" + QString(e.what()));
            Log::writeToLog(m_db.path, m_dclFacade->currentSession().username,
                           QString(e.what()));
        }
    }
    // DDL: CREATE TABLE
    else if (sql.startsWith("CREATE TABLE", Qt::CaseInsensitive)) {
        try {
            Lexer l;
            QList<Token> ts = l.ReadSQL(sql);
            for (auto t : ts) {
                qDebug() << "字段名:" << t.text << "类型:" << t.type;
            }
            m_table = m_parser.parseCreateTable(sql, m_db);
            DDL::writeToDbs(m_db, m_table);
            DDL::saveSchema(m_table, m_db.path);
            m_sqlPanel->appendOutput("建表成功");
            Log::writeToLog(m_db.path, m_dclFacade->currentSession().username, sql);
            m_dbExplorer->refreshTree();
            m_sqlPanel->clearInput();
        } catch (const std::invalid_argument& e) {
            m_sqlPanel->appendOutput("SQL执行失败：" + QString(e.what()));
            Log::writeToLog(m_db.path, m_dclFacade->currentSession().username,
                           QString(e.what()));
        }
    }
    // DDL: ALTER TABLE
    else if (sql.startsWith("ALTER TABLE", Qt::CaseInsensitive)) {
        try {
            QString temp = sql.toLower();
            temp.replace("\n", "");
            temp.replace("\r", "");
            temp.replace(" ", "");
            if (temp.contains("add")) {
                if (temp.contains("addconstraint")) {
                    m_parser.paraseAddCS(sql, m_db);
                    m_sqlPanel->appendOutput("添加约束成功");
                    Log::writeToLog(m_db.path, m_dclFacade->currentSession().username, sql);
                    m_dbExplorer->refreshTree();
                    m_sqlPanel->clearInput();
                } else {
                    m_parser.paraseAddCol(sql, m_db);
                    m_sqlPanel->appendOutput("添加字段成功");
                    Log::writeToLog(m_db.path, m_dclFacade->currentSession().username, sql);
                    m_dbExplorer->refreshTree();
                    m_sqlPanel->clearInput();
                }
            }
            if (temp.contains("drop")) {
                if (temp.contains("dropcolumn")) {
                    m_parser.paraseDTableF(sql, m_db.path, m_db);
                    m_sqlPanel->appendOutput("删除字段成功");
                    Log::writeToLog(m_db.path, m_dclFacade->currentSession().username, sql);
                    m_dbExplorer->refreshTree();
                    m_sqlPanel->clearInput();
                } else {
                    m_parser.paraseDTKEY(sql, m_db);
                    m_sqlPanel->appendOutput("删除约束成功");
                    Log::writeToLog(m_db.path, m_dclFacade->currentSession().username, sql);
                    m_dbExplorer->refreshTree();
                    m_sqlPanel->clearInput();
                }
            }
            if (temp.contains("modify")) {
                m_parser.paraseModifyCol(sql, m_db);
                m_sqlPanel->appendOutput("修改成功");
                Log::writeToLog(m_db.path, m_dclFacade->currentSession().username, sql);
                m_dbExplorer->refreshTree();
                m_sqlPanel->clearInput();
            }
            if (temp.contains("change")) {
                m_parser.paraseChangeCol(sql, m_db);
                m_sqlPanel->appendOutput("修改成功");
                Log::writeToLog(m_db.path, m_dclFacade->currentSession().username, sql);
                m_dbExplorer->refreshTree();
                m_sqlPanel->clearInput();
            }
        } catch (const std::invalid_argument& e) {
            m_sqlPanel->appendOutput("SQL执行失败：" + QString(e.what()));
            Log::writeToLog(m_db.path, m_dclFacade->currentSession().username,
                           QString(e.what()));
        }
    }
    // DDL: DROP TABLE
    else if (sql.startsWith("DROP TABLE", Qt::CaseInsensitive)) {
        try {
            m_parser.paraseDropTable(sql, m_db);
            m_sqlPanel->appendOutput("删除成功");
            Log::writeToLog(m_db.path, m_dclFacade->currentSession().username, sql);
            m_dbExplorer->refreshTree();
            m_sqlPanel->clearInput();
        } catch (const std::invalid_argument& e) {
            m_sqlPanel->appendOutput("SQL执行失败：" + QString(e.what()));
            Log::writeToLog(m_db.path, m_dclFacade->currentSession().username,
                           QString(e.what()));
        }
    }
    // DML: INSERT
    else if (sql.startsWith("INSERT", Qt::CaseInsensitive)) {
        try {
            InsertStatement stmt = m_parser.parseInsert(sql);
            int affected = DML::executeInsert(m_db, stmt);
            m_sqlPanel->appendOutput(QString("插入成功，影响 %1 行").arg(affected));
            Log::writeToLog(m_db.path, m_dclFacade->currentSession().username, sql);
        } catch (const std::invalid_argument& e) {
            m_sqlPanel->appendOutput(QString("SQL语句执行失败：%1").arg(e.what()));
            Log::writeToLog(m_db.path, m_dclFacade->currentSession().username,
                           QString("SQL语句执行失败：%1").arg(e.what()));
        }
    }
    // DML: UPDATE
    else if (sql.startsWith("UPDATE", Qt::CaseInsensitive)) {
        try {
            UpdateStatement stmt = m_parser.parseUpdate(sql);
            int affected = DML::executeUpdate(m_db, stmt);
            m_sqlPanel->appendOutput(QString("更新成功，影响 %1 行").arg(affected));
            Log::writeToLog(m_db.path, m_dclFacade->currentSession().username, sql);
        } catch (const std::invalid_argument& e) {
            m_sqlPanel->appendOutput(QString("SQL语句执行失败：%1").arg(e.what()));
            Log::writeToLog(m_db.path, m_dclFacade->currentSession().username,
                           QString("SQL语句执行失败：%1").arg(e.what()));
        }
    }
    // DML: DELETE
    else if (sql.startsWith("DELETE", Qt::CaseInsensitive)) {
        try {
            DeleteStatement stmt = m_parser.parseDelete(sql);
            int affected = DML::executeDelete(m_db, stmt);
            m_sqlPanel->appendOutput(QString("删除成功，影响 %1 行").arg(affected));
            Log::writeToLog(m_db.path, m_dclFacade->currentSession().username, sql);
        } catch (const std::invalid_argument& e) {
            m_sqlPanel->appendOutput(QString("SQL语句执行失败：%1").arg(e.what()));
            Log::writeToLog(m_db.path, m_dclFacade->currentSession().username,
                           QString("SQL语句执行失败：%1").arg(e.what()));
        }
    }
    // DML: SELECT
    else if (sql.startsWith("SELECT", Qt::CaseInsensitive)) {
        try {
            SelectStatement stmt = m_parser.parseSelect(sql);
            QString result = DML::executeSelect(m_db, stmt);
            m_sqlPanel->appendOutput(result);
            Log::writeToLog(m_db.path, m_dclFacade->currentSession().username, sql);
        } catch (const std::invalid_argument& e) {
            m_sqlPanel->appendOutput(QString("SQL语句执行失败：%1").arg(e.what()));
            Log::writeToLog(m_db.path, m_dclFacade->currentSession().username,
                           QString("SQL语句执行失败：%1").arg(e.what()));
        }
    }

    // Add to history
    m_historyPanel->addQuery(sql);
}

// ---- Path Setting ----

void MainWindow::handleSetPath()
{
    Dialog* dialog = new Dialog(this);
    dialog->setModal(false);
    dialog->show();

    connect(dialog, &QDialog::accepted, this, [=]() {
        QString path = dialog->ui->lineEdit->text();
        qDebug() << "获取数据库存储路径" << path;
        m_dbPath = path;
    });
}

// ---- Table Schema View ----

void MainWindow::openTableSchema(const QString& dbName, const QString& tableName)
{
    ResultView* view = new ResultView(&m_parser, this);
    view->showTableSchema(dbName, tableName);

    connect(view, &ResultView::backToEditor, this, [this, view]() {
        int idx = m_centralTabs->indexOf(view);
        if (idx >= 0) {
            m_centralTabs->removeTab(idx);
        }
        view->deleteLater();
    });

    int idx = m_centralTabs->addTab(view,
        QString::fromUtf8("表结构: %1.%2").arg(dbName, tableName));
    m_centralTabs->setCurrentIndex(idx);
}

// ---- Table Data View ----

void MainWindow::openTableData(const QString& dbName, const QString& tableName)
{
    ResultView* view = new ResultView(&m_parser, this);
    view->showTableData(dbName, tableName);

    connect(view, &ResultView::backToEditor, this, [this, view]() {
        int idx = m_centralTabs->indexOf(view);
        if (idx >= 0) {
            m_centralTabs->removeTab(idx);
        }
        view->deleteLater();
    });

    int idx = m_centralTabs->addTab(view,
        QString::fromUtf8("表数据: %1.%2").arg(dbName, tableName));
    m_centralTabs->setCurrentIndex(idx);
}

// ---- Delete Table ----

void MainWindow::handleDeleteTable(const QString& dbName, const QString& tableName)
{
    auto btn = QMessageBox::question(this, "删除",
                                     "确定删除表：" + tableName + "？");

    if (btn != QMessageBox::Yes) return;

    QString dbPath = m_parser.getDbPathByName(dbName);
    QString tbPath = dbPath + "/" + tableName;
    QString dbsPath = dbPath + "/" + dbName + ".dbs";
    QVector<QString> tableNames;

    QStringList tames = DDL::readFromDbs(dbsPath);
    if (!tames.empty()) {
        for (QString name : tames) {
            tableNames.append(name);
        }
    }

    QDir dir(tbPath);
    dir.removeRecursively();

    for (int i = 0; i < tableNames.size(); i++) {
        if (tableNames[i] == tableName) {
            tableNames.remove(i);
            break;
        }
    }

    QFile f(dbsPath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qDebug() << "写入DBS失败";
        return;
    }
    QDataStream out(&f);
    for (QString name : tableNames) {
        qDebug() << "重新写入的表名:" << name;
        out << name;
    }
    f.close();

    m_sqlPanel->appendOutput("表 " + tableName + " 已删除");
    m_dbExplorer->refreshTree();
}

// ---- Create Table (stub) ----

void MainWindow::handleCreateTable(const QString& dbName)
{
    m_sqlPanel->appendOutput(QString::fromUtf8("右键 → 新建表（数据库：") + dbName + ")");
}

// ---- Modify Table (stub) ----

void MainWindow::handleModifyTable(const QString& dbName, const QString& tableName)
{
    Q_UNUSED(dbName);
    m_sqlPanel->appendOutput(QString::fromUtf8("右键 → 修改表结构：") + tableName);
}

// ---- Dialog Placeholders ----

void MainWindow::showSettings()
{
    SettingsDialog dlg(this);
    dlg.exec();
}

void MainWindow::showAbout()
{
    AboutDialog dlg(this);
    dlg.exec();
}

void MainWindow::showExportPlaceholder()
{
    QMessageBox::information(this,
        QString::fromUtf8("导出"),
        QString::fromUtf8("导出功能即将推出！"));
}

// ---- Query History ----

void MainWindow::onHistoryQuerySelected(const QString& sql)
{
    m_sqlPanel->setSql(sql);
}
