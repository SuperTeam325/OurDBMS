#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "dialog.h"
#include "./ui_dialog.h"
#include "Log.h"
#include "Lexer.h"
#include <QDir>
#include <QMenu>
#include <QMessageBox>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>
#include <QSplitter>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QDateTime>
#include <QLineEdit>
#include <QTextStream>
#include "DML.h"
#include "Index.h"
#include "DCL/dcl_facade.h"
#include "../DQL/DQL.h"

MainWindow::MainWindow(DCL::DclFacade* facade, QWidget *parent)
    : QMainWindow(parent)
    , dclFacade(facade)
    , ui(new Ui::MainWindow)
{
    QString buildDir = QDir::currentPath();
    qDebug() << "项目根目录：" << PROJECT_ROOT_DIR;

    ui->setupUi(this);

    // 用 QSplitter 替换固定布局，使导航栏可拖拽拉伸
    QSplitter* mainSplitter = new QSplitter(Qt::Horizontal, this);
    ui->centralwidget->layout()->removeWidget(ui->navFrame);
    ui->centralwidget->layout()->removeWidget(ui->contentStack);
    mainSplitter->addWidget(ui->navFrame);
    mainSplitter->addWidget(ui->contentStack);
    mainSplitter->setStretchFactor(0, 0);
    mainSplitter->setStretchFactor(1, 1);
    mainSplitter->setHandleWidth(4);
    mainSplitter->setChildrenCollapsible(false);
    delete ui->centralwidget->layout();
    QVBoxLayout* wrapper = new QVBoxLayout(ui->centralwidget);
    wrapper->setContentsMargins(0, 0, 0, 0);
    wrapper->addWidget(mainSplitter);

    // 设置用户信息
    if (dclFacade && dclFacade->isLoggedIn()) {
        setWindowTitle("DBMS - 用户: " + dclFacade->currentSession().username);
        ui->currentUserLabel->setText("当前用户: " + dclFacade->currentSession().username);
    } else {
        setWindowTitle("DBMS - 未登录");
        ui->currentUserLabel->setText("未登录用户");
    }

    // 初始化数据管理页
    displayDB();
    ui->treeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->treeWidget, &QTreeWidget::customContextMenuRequested,
            this, &MainWindow::onTreeRightClicked);

    // 显式连接日志列表点击（手动 connect，不再依赖 auto-connect）
    connect(ui->logFileList, &QListWidget::itemClicked,
            this, &MainWindow::onLogFileItemClicked);
    connect(ui->logSearchInput, &QLineEdit::textChanged,
            this, &MainWindow::onLogSearchChanged);

    // 用 QPalette 统一子页面背景色（CSS 选择器对 QStackedWidget 子页不生效）
    QPalette pagePal;
    pagePal.setColor(QPalette::Window, QColor("#f5f6fa"));
    ui->detailStack->widget(0)->setAutoFillBackground(true);
    ui->detailStack->widget(0)->setPalette(pagePal);
    ui->detailStack->widget(1)->setAutoFillBackground(true);
    ui->detailStack->widget(1)->setPalette(pagePal);
    ui->detailStack->widget(2)->setAutoFillBackground(true);
    ui->detailStack->widget(2)->setPalette(pagePal);

    // 默认显示数据管理页
    ui->contentStack->setCurrentIndex(0);
    ui->detailStack->setCurrentIndex(0);
}

MainWindow::~MainWindow()
{
    delete ui;
}

// ==================== 导航切换 ====================

void MainWindow::setNavButtonChecked(QPushButton* active)
{
    ui->btnDataMgmt->setChecked(active == ui->btnDataMgmt);
    ui->btnConsole->setChecked(active == ui->btnConsole);
    ui->btnLogViewer->setChecked(active == ui->btnLogViewer);
}

void MainWindow::on_btnDataMgmt_clicked()
{
    setNavButtonChecked(ui->btnDataMgmt);
    ui->contentStack->setCurrentIndex(0);
}

void MainWindow::on_btnConsole_clicked()
{
    setNavButtonChecked(ui->btnConsole);
    ui->contentStack->setCurrentIndex(1);
}

void MainWindow::on_btnLogViewer_clicked()
{
    setNavButtonChecked(ui->btnLogViewer);
    ui->contentStack->setCurrentIndex(2);
    loadLogFileList();
}

// ==================== 数据管理 ====================

void MainWindow::on_btnRefreshTree_clicked()
{
    refreshDBTreeWithState();
}

// 获取所有数据库的根目录
QSet<QString> getAllDbRootPaths()
{
    QFile file("db_config.json");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return {};
    }
    QByteArray data = file.readAll();
    file.close();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonObject obj = doc.object();
    QSet<QString> rootPaths;
    for (auto it = obj.begin(); it != obj.end(); ++it) {
        QString dbPath = it.value().toString();
        QString rootPath = QFileInfo(dbPath).absolutePath();
        rootPaths.insert(rootPath);
    }
    return rootPaths;
}

void MainWindow::displayDB()
{
    ui->treeWidget->clear();

    QSet<QString> paths = getAllDbRootPaths();
    QFileInfoList AllDbDirList;
    for (auto const p : paths) {
        QDir rootDir(p);
        QFileInfoList dbDirList = rootDir.entryInfoList(
            QDir::Dirs | QDir::NoDotAndDotDot);
        AllDbDirList.append(dbDirList);
    }

    QTreeWidgetItem *normalDbGroup = new QTreeWidgetItem(ui->treeWidget);
    normalDbGroup->setText(0, "数据库");

    QTreeWidgetItem *systemDbGroup = new QTreeWidgetItem(ui->treeWidget);
    systemDbGroup->setText(0, "系统数据库");

    QTreeWidgetItem *UserGroup = new QTreeWidgetItem(ui->treeWidget);
    UserGroup->setText(0, "用户");

    for (QFileInfo dbInfo : AllDbDirList) {
        QString dbName = dbInfo.fileName();
        QTreeWidgetItem *targetGroup;

        if (dbName == "sys") {
            targetGroup = systemDbGroup;
        } else {
            targetGroup = normalDbGroup;
        }

        QTreeWidgetItem *dbItem = new QTreeWidgetItem(targetGroup);
        dbItem->setText(0, dbName);

        QTreeWidgetItem *tableGroupItem = new QTreeWidgetItem(dbItem);
        tableGroupItem->setText(0, "表");

        QDir dbFolder(dbInfo.absoluteFilePath());
        QFileInfoList tableDirList = dbFolder.entryInfoList(
            QDir::Dirs | QDir::NoDotAndDotDot);

        for (QFileInfo tableInfo : tableDirList) {
            QString tableName = tableInfo.fileName();

            if (tableInfo.fileName() == "logs") {
                continue;
            }

            QTreeWidgetItem *tableItem = new QTreeWidgetItem(tableGroupItem);
            tableItem->setText(0, tableName);

            QString tbsPath = tableInfo.absoluteFilePath() + "/" + tableName + ".tbs";
            DDL::Table table = DDL::loadSchema(tbsPath);

            QTreeWidgetItem *colGroupItem = new QTreeWidgetItem(tableItem);
            colGroupItem->setText(0, "列");
            for (const DDL::Field& f : table.fields) {
                QString fieldText = QString("%1 (%2, %3)")
                    .arg(f.field_name)
                    .arg(DDL::fieldTypeToString(f.field_type))
                    .arg(f.length);
                QTreeWidgetItem *fieldItem = new QTreeWidgetItem(colGroupItem);
                fieldItem->setText(0, fieldText);
            }

            QTreeWidgetItem *ConstGroupItem = new QTreeWidgetItem(tableItem);
            ConstGroupItem->setText(0, "约束");
            QVector<TokenType> CSType = {TOKEN_NOT, TOKEN_DEFAULT, TOKEN_PRIMARY, TOKEN_UNIQUE, TOKEN_AUTO_INCREMENT, TOKEN_FOREIGN};
            for (const DDL::Field& f : table.fields) {
                for (auto cst : CSType) {
                    if (!f.field_Constraint.Const_Name[cst].isEmpty()) {
                        QString CSText = QString("%1(%2)")
                            .arg(f.field_Constraint.Const_Name[cst])
                            .arg(f.field_Constraint.toString(cst));
                        QTreeWidgetItem *CsItem = new QTreeWidgetItem(ConstGroupItem);
                        CsItem->setText(0, CSText);
                    }
                }
            }
        }
    }

    // 加载用户信息
    QString path = PROJECT_ROOT_DIR "/dataDB";
    QVector<QVector<QString>> users = DDL::loadTableData(userReposity.usersTable(), path + "/sys");
    QVector<QVector<QString>> permission = DDL::loadTableData(userPermission.permissionsTable(), path + "/sys");

    for (int i = 0; i < users.size(); ++i) {
        QTreeWidgetItem *NameItem = new QTreeWidgetItem(UserGroup);
        NameItem->setText(0, users[i][0]);
        QTreeWidgetItem *PItem = new QTreeWidgetItem(NameItem);
        PItem->setText(0, "授权");

        if (users[i][3] == "1") {
            QTreeWidgetItem *item = new QTreeWidgetItem(PItem);
            item->setText(0, "ALL PRIVILEGES");
        }
        for (int j = 0; j < permission.size(); ++j) {
            if (users[i][0] == permission[j][0]) {
                QTreeWidgetItem *item1 = new QTreeWidgetItem(PItem);
                item1->setText(0, QString(permission[j][3] + "(%1,%2)").arg(permission[j][1]).arg(permission[j][2]));
            }
        }
    }
}

QStringList MainWindow::saveExpandedPaths(QTreeWidgetItem *item, const QString &parentPath)
{
    QStringList paths;
    if (!item) return paths;
    QString currentPath = parentPath.isEmpty() ? item->text(0) : parentPath + "/" + item->text(0);
    if (item->isExpanded()) {
        paths.append(currentPath);
    }
    for (int i = 0; i < item->childCount(); ++i) {
        paths += saveExpandedPaths(item->child(i), currentPath);
    }
    return paths;
}

void MainWindow::restoreExpandedPaths(QTreeWidgetItem *item, const QString &parentPath, const QStringList &paths)
{
    if (!item) return;
    QString currentPath = parentPath.isEmpty() ? item->text(0) : parentPath + "/" + item->text(0);
    if (paths.contains(currentPath)) {
        item->setExpanded(true);
    }
    for (int i = 0; i < item->childCount(); ++i) {
        restoreExpandedPaths(item->child(i), currentPath, paths);
    }
}

void MainWindow::refreshDBTreeWithState()
{
    QStringList expandedPaths;
    for (int i = 0; i < ui->treeWidget->topLevelItemCount(); ++i) {
        expandedPaths += saveExpandedPaths(ui->treeWidget->topLevelItem(i), "");
    }
    displayDB();
    for (int i = 0; i < ui->treeWidget->topLevelItemCount(); ++i) {
        restoreExpandedPaths(ui->treeWidget->topLevelItem(i), "", expandedPaths);
    }
}

// ==================== 右键菜单 ====================

void MainWindow::onTreeRightClicked(const QPoint &pos)
{
    QTreeWidgetItem *item = ui->treeWidget->itemAt(pos);
    if (!item) return;

    QMenu menu(this);

    QString now = item->text(0);
    QTreeWidgetItem *parent = item->parent();
    QString pText = parent ? parent->text(0) : "";

    bool isDatabaseNode = (pText == "数据库");
    bool isTableNode    = (pText == "表");
    bool isColOrConstNode = (pText == "列" || pText == "约束");

    qDebug() << item->text(0);

    if (isDatabaseNode) {
        if (item->text(0) != "sys") {
            menu.addAction("新建表", this, &MainWindow::createTableMenu);
        }
        if (dclFacade->currentSession().isAdmin) {
            menu.addAction("删除数据库", this, &MainWindow::deleteDatabase);
        }
        menu.addSeparator();
        menu.addAction("刷新", this, &MainWindow::refreshTree);
    } else if (isTableNode) {
        menu.addAction("查看表", this, &MainWindow::viewTableMenu);
        menu.addAction("查看数据", this, &MainWindow::viewTableDataMenu);
        if (item->parent()->parent()->text(0) != "sys") {
            menu.addAction("修改表结构", this, &MainWindow::modifyTableMenu);
            menu.addAction("删除表", this, &MainWindow::deleteTableMenu);
        }
        menu.addSeparator();
        menu.addAction("刷新", this, &MainWindow::refreshTree);
    } else if (isColOrConstNode) {
        menu.addAction("查看详情");
        menu.addSeparator();
        menu.addAction("刷新", this, &MainWindow::refreshTree);
    } else {
        menu.addAction("刷新", this, &MainWindow::refreshTree);
    }

    menu.exec(ui->treeWidget->viewport()->mapToGlobal(pos));
}

void MainWindow::refreshTree()
{
    refreshDBTreeWithState();
}

// ==================== 查看表结构 ====================

void MainWindow::viewTableMenu()
{
    QTreeWidgetItem *item = ui->treeWidget->currentItem();
    if (!item) return;

    QString tableName = item->text(0);
    QString dbName = item->parent()->parent()->text(0);
    qDebug()<<dbName;
    QString dbPath = p.getDbPathByName(dbName);
    QString tbsPath = dbPath + "/" + tableName + "/" + tableName + ".tbs";
    DDL::Table table = DDL::loadSchema(tbsPath);

    // 使用 detailStack 的 page 1（表结构页）
    QWidget *page = ui->detailStack->widget(1);

    if (page->layout() != nullptr) {
        QLayoutItem *child;
        while ((child = page->layout()->takeAt(0)) != nullptr) {
            delete child->widget();
            delete child;
        }
        delete page->layout();
    }

    QVBoxLayout *mainLayout = new QVBoxLayout(page);
    mainLayout->setContentsMargins(10, 10, 10, 15);
    mainLayout->setSpacing(10);

    // 标题
    QLabel *titleLabel = new QLabel(QString("表结构: %1.%2").arg(dbName, tableName));
    titleLabel->setStyleSheet("font-size: 12pt; font-weight: bold; color: #2c3e50;");
    mainLayout->addWidget(titleLabel);

    QTableWidget *tw = new QTableWidget;
    tw->setColumnCount(4);
    tw->setHorizontalHeaderLabels({"字段名", "字段类型", "长度", "约束"});
    tw->setShowGrid(true);
    tw->setAlternatingRowColors(true);
    tw->setStyleSheet(R"(
        QTableWidget {
            gridline-color: #d0d0d0;
            font-size: 10pt;
        }
        QHeaderView::section {
            background-color: #e0e0e0;
            border: 1px solid #c0c0c0;
            font-weight: bold;
            padding: 4px;
        }
        QTableWidget::item {
            border: 1px solid #d0d0d0;
            padding: 4px;
        }
        QTableWidget::item:selected {
            background-color: transparent;
            color: black;
        }
    )");
    tw->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    tw->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    tw->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

    QVector<TokenType> CSType = {TOKEN_NOT, TOKEN_DEFAULT, TOKEN_PRIMARY, TOKEN_UNIQUE, TOKEN_AUTO_INCREMENT, TOKEN_FOREIGN};
    for (const DDL::Field &f : table.fields) {
        int r = tw->rowCount();
        tw->insertRow(r);
        tw->setItem(r, 0, new QTableWidgetItem(f.field_name));
        tw->setItem(r, 1, new QTableWidgetItem(DDL::fieldTypeToString(f.field_type)));
        tw->setItem(r, 2, new QTableWidgetItem(QString::number(f.length)));

        QString cons;
        for (auto c : CSType) {
            if (!f.field_Constraint.Const_Name[c].isEmpty())
                cons += f.field_Constraint.Const_Name[c] + "(" + f.field_Constraint.toString(c) + ")\n";
        }
        tw->setItem(r, 3, new QTableWidgetItem(cons.trimmed()));

        for (int c = 0; c < 4; c++)
            tw->item(r, c)->setTextAlignment(Qt::AlignCenter);
    }

    QPushButton *btnBack = new QPushButton("返回");
    btnBack->setMinimumWidth(200);
    btnBack->setStyleSheet(R"(
        QPushButton {
            font-size: 12pt;
            padding: 12px 30px;
        }
    )");
    connect(btnBack, &QPushButton::clicked, this, [=]() {
        ui->detailStack->setCurrentIndex(0);
    });

    mainLayout->addWidget(tw);
    mainLayout->addWidget(btnBack, 0, Qt::AlignCenter);

    ui->detailStack->setCurrentIndex(1);
}

// ==================== 查看表数据 ====================

void MainWindow::viewTableDataMenu()
{
    QTreeWidgetItem *item = ui->treeWidget->currentItem();
    if (!item) return;

    QString tableName = item->text(0);
    QString dbName = item->parent()->parent()->text(0);
    QString dbPath = p.getDbPathByName(dbName);

    QString tbsPath = dbPath + "/" + tableName + "/" + tableName + ".tbs";
    DDL::Table table = DDL::loadSchema(tbsPath);

    DDL::DataBase db;
    db.name = dbName;
    db.path = dbPath;

    QVector<QVector<QString>> tableData = DML::loadTableRows(db, table);

    // 使用 detailStack 的 page 2（表数据页）
    QWidget *page = ui->detailStack->widget(2);

    if (page->layout() != nullptr) {
        QLayoutItem *child;
        while ((child = page->layout()->takeAt(0)) != nullptr) {
            delete child->widget();
            delete child;
        }
        delete page->layout();
    }

    QVBoxLayout *mainLayout = new QVBoxLayout(page);
    mainLayout->setContentsMargins(10, 10, 10, 15);
    mainLayout->setSpacing(10);

    // 标题
    QLabel *titleLabel = new QLabel(QString("表数据: %1.%2 (%3 行)").arg(dbName, tableName).arg(tableData.size()));
    titleLabel->setStyleSheet("font-size: 12pt; font-weight: bold; color: #2c3e50;");
    mainLayout->addWidget(titleLabel);

    QTableWidget *tw = new QTableWidget;
    int columnCount = table.fields.size();
    int rowCount = tableData.size();

    tw->setColumnCount(columnCount);
    tw->setRowCount(rowCount);

    QStringList headers;
    for (const DDL::Field& f : table.fields) {
        headers << f.field_name;
    }
    tw->setHorizontalHeaderLabels(headers);

    tw->setShowGrid(true);
    tw->setAlternatingRowColors(true);
    tw->setStyleSheet(R"(
        QTableWidget {
            gridline-color: #d0d0d0;
            font-size: 10pt;
        }
        QHeaderView::section {
            background-color: #e0e0e0;
            border: 1px solid #c0c0c0;
            font-weight: bold;
            padding: 4px;
        }
        QTableWidget::item {
            border: 1px solid #d0d0d0;
            padding: 4px;
        }
        QTableWidget::item:selected {
            background-color: transparent;
            color: black;
        }
    )");

    tw->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    tw->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    tw->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

    for (int row = 0; row < tableData.size(); row++) {
        const QVector<QString>& rowData = tableData[row];
        for (int col = 0; col < rowData.size(); col++) {
            tw->setItem(row, col, new QTableWidgetItem(rowData[col]));
            tw->item(row, col)->setTextAlignment(Qt::AlignCenter);
        }
    }

    if (tableData.isEmpty()) {
        tw->setRowCount(1);
        tw->setItem(0, 0, new QTableWidgetItem("(表数据为空)"));
        tw->item(0, 0)->setTextAlignment(Qt::AlignCenter);
    }

    QPushButton *btnBack = new QPushButton("返回");
    btnBack->setMinimumWidth(200);
    btnBack->setStyleSheet(R"(
        QPushButton {
            font-size: 12pt;
            padding: 12px 30px;
        }
    )");

    connect(btnBack, &QPushButton::clicked, this, [=]() {
        ui->detailStack->setCurrentIndex(0);
    });

    mainLayout->addWidget(tw);
    mainLayout->addWidget(btnBack, 0, Qt::AlignCenter);

    ui->detailStack->setCurrentIndex(2);
}

// ==================== 删除数据库 ====================

void MainWindow::deleteDatabase()
{
    QFile file("db_config.json");
    if (!file.open(QIODevice::ReadWrite | QIODevice::Text)) {
        throw std::invalid_argument("数据库文件出错");
    }

    QByteArray data = file.readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        file.close();
    }

    QJsonObject obj = doc.object();
    file.close();

    QTreeWidgetItem *item = ui->treeWidget->currentItem();
    QString dbName = item->text(0);

    auto btn = QMessageBox::question(this, "删除", "确定删除：" + dbName + "？");

    if (btn == QMessageBox::Yes) {
        if (!obj.contains(dbName)) {
            ui->Terminal->append("错误：这不是一个有效的数据库！");
            return;
        }

        QString path = obj[dbName].toString();
        obj.remove(dbName);

        QDir dbDir(path);
        if (dbDir.exists()) {
            dbDir.removeRecursively();
        }

        if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
            throw std::invalid_argument("写入配置文件失败");
        }

        doc.setObject(obj);
        file.seek(0);
        file.write(doc.toJson());
        file.close();
    }

    refreshTree();
}

void MainWindow::createTableMenu()
{
    QTreeWidgetItem *item = ui->treeWidget->currentItem();
    if (!item) return;
    QString dbName = item->text(0);
    ui->Terminal->append("右键 -> 新建表（数据库：" + dbName + "）");
    // 切换到控制台以便用户输入 CREATE TABLE 语句
    on_btnConsole_clicked();
}

void MainWindow::deleteTableMenu()
{
    QTreeWidgetItem *item = ui->treeWidget->currentItem();
    if (!item) return;

    QString tableName = item->text(0);
    auto btn = QMessageBox::question(this, "删除", "确定删除表：" + tableName + "？");

    if (btn == QMessageBox::Yes) {
        QTreeWidgetItem *item = ui->treeWidget->currentItem();
        if (!item) return;

        QString tableName = item->text(0);
        QString dbName = item->parent()->parent()->text(0);

        QString dbPath = p.getDbPathByName(dbName);
        QString tbPath = dbPath + "/" + tableName;
        QString dbsPath = dbPath + "/" + dbName + ".dbs";
        QVector<QString> tableNames;

        tableNames.clear();
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

        QFile f(dbPath + "/" + dbName + ".dbs");
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

        ui->Terminal->append("表 " + tableName + " 已删除");
        refreshTree();
    }
}

void MainWindow::modifyTableMenu()
{
    QTreeWidgetItem *item = ui->treeWidget->currentItem();
    if (!item) return;
    QString tableName = item->text(0);
    ui->Terminal->append("右键 -> 修改表结构：" + tableName);
    // 切换到控制台以便用户输入 ALTER TABLE 语句
    on_btnConsole_clicked();
}

// ==================== 设置数据库路径 ====================

void MainWindow::on_btnSetPath_clicked()
{
    Dialog *dialog = new Dialog(this);
    dialog->setModal(false);
    dialog->show();

    connect(dialog, &QDialog::accepted, this, [=]() {
        QString path = dialog->ui->lineEdit->text();
        qDebug() << "获取数据库存储路径" << path;
        DBpath = path;
    });
}

// ==================== SQL 执行 ====================

void MainWindow::on_SubmitSQL_clicked()
{
    QString sql = ui->sqlEdit->toPlainText().trimmed();
    if (sql.isEmpty()) {
        return;
    }

    // 优先处理 DCL 语句
    if (dclFacade) {
        QString dclMessage;
        QString dclError;
        if (dclFacade->tryHandleSessionSql(sql, dclMessage, dclError)) {
            if (!dclError.isEmpty()) {
                ui->Terminal->append("SQL执行失败：" + dclError);
            } else {
                ui->Terminal->append(dclMessage);
                if (dclFacade->isLoggedIn()) {
                    setWindowTitle("DBMS - 用户: " + dclFacade->currentSession().username);
                    ui->currentUserLabel->setText("当前用户: " + dclFacade->currentSession().username);
                } else {
                    setWindowTitle("DBMS - 未登录");
                    ui->currentUserLabel->setText("未登录用户");
                }
            }
            ui->sqlEdit->clear();
            return;
        }
    }

    // 拒绝未登录用户的操作
    if (!dclFacade || !dclFacade->isLoggedIn()) {
        ui->Terminal->append("SQL执行失败：未登录");
        return;
    }

    // 拒绝无权限用户的操作
    QString authError;
    if (!dclFacade->authorizeSql(sql, authError)) {
        ui->Terminal->append("SQL执行失败：" + authError);
        return;
    }

    // 索引语句
    QString sqlUpper = sql.toUpper().trimmed();
    if (sqlUpper.startsWith("CREATE INDEX") || sqlUpper.startsWith("CREATE UNIQUE INDEX")) {
        try {
            if (db.path.isEmpty()) {
                throw std::invalid_argument("未选择数据库");
            }
            IndexMeta idxMeta = p.parseCreateIndex(sql, db);

            // 从 SQL 中提取表名
            QString tableName;
            QList<Token> ts = Lexer().ReadSQL(sql);
            for (int i = 0; i < ts.size(); i++) {
                if (ts[i].type == TOKEN_ON && i + 1 < ts.size()) {
                    tableName = ts[i + 1].text;
                    break;
                }
            }

            DDL::Table table = DDL::loadSchema(db.path + "/" + tableName + "/" + tableName + ".tbs");
            QVector<QVector<QString>> rows = DML::loadTableRows(db, table);

            IndexManager im(db.path, tableName);
            if (!im.createIndex(table, idxMeta, rows)) {
                throw std::invalid_argument("索引创建失败");
            }

            table.indexes.append(idxMeta);
            DDL::saveSchema(table, db.path);
            ui->Terminal->append("索引 " + idxMeta.name + " 创建成功");
            Log::writeToLog(db.path, dclFacade->currentSession().username, sql);
            refreshDBTreeWithState();
            ui->sqlEdit->clear();
        } catch (const std::invalid_argument& e) {
            ui->Terminal->append("SQL执行失败：" + QString(e.what()));
        }
        return;
    }

    if (sqlUpper.startsWith("DROP INDEX")) {
        try {
            if (db.path.isEmpty()) {
                throw std::invalid_argument("未选择数据库");
            }
            p.parseDropIndex(sql, db);
            ui->Terminal->append("索引删除成功");
            Log::writeToLog(db.path, dclFacade->currentSession().username, sql);
            refreshDBTreeWithState();
            ui->sqlEdit->clear();
        } catch (const std::invalid_argument& e) {
            ui->Terminal->append("SQL执行失败：" + QString(e.what()));
        }
        return;
    }

    // DDL
    if (sql.startsWith("CREATE DATABASE", Qt::CaseInsensitive)) {
        try {
            p.paraseCreateDB(sql, DBpath);
            ui->Terminal->append("数据库创建成功");
            refreshDBTreeWithState();
            ui->sqlEdit->clear();
        } catch (const std::invalid_argument& e) {
            ui->Terminal->append("SQL执行失败：" + QString(e.what()));
        }
    }
    else if (sql.startsWith("USE", Qt::CaseInsensitive)) {
        try {
            p.paraseUSEDB(sql, db);
            dclFacade->setCurrentDatabase(db.name);
            ui->Terminal->append(QString("切换成功，当前数据库：%1").arg(db.name));
            Log::writeToLog(db.path, dclFacade->currentSession().username, sql);
            ui->sqlEdit->clear();
        } catch (const std::invalid_argument& e) {
            ui->Terminal->append("SQL执行失败：" + QString(e.what()));
            Log::writeToLog(db.path, dclFacade->currentSession().username, QString(e.what()));
        }
    }
    else if (sql.startsWith("CREATE TABLE", Qt::CaseInsensitive)) {
        try {
            Lexer l;
            QList<Token> ts = l.ReadSQL(sql);
            for (auto t : ts) {
                qDebug() << "字段名:" << t.text << "类型:" << t.type;
            }
            t = p.parseCreateTable(sql, db);
            DDL::writeToDbs(db, t);
            DDL::saveSchema(t, db.path);
            // 自动构建索引（PK、UNIQUE 列）
            if (!t.indexes.isEmpty()) {
                IndexManager im(db.path, t.name);
                QVector<QVector<QString>> emptyRows;
                for (const IndexMeta& meta : t.indexes) {
                    im.createIndex(t, meta, emptyRows);
                }
            }
            ui->Terminal->append("建表成功");
            Log::writeToLog(db.path, dclFacade->currentSession().username, sql);
            refreshDBTreeWithState();
            ui->sqlEdit->clear();
        } catch (const std::invalid_argument& e) {
            ui->Terminal->append("SQL执行失败：" + QString(e.what()));
            Log::writeToLog(db.path, dclFacade->currentSession().username, QString(e.what()));
        }
    }
    else if (sql.startsWith("ALTER TABLE", Qt::CaseInsensitive)) {
        try {
            QString lowerSql = sql.toLower();
            QString temp = sql.toLower();
            temp.replace("\n", "");
            temp.replace("\r", "");
            temp.replace(" ", "");
            if (temp.contains("add")) {
                if (temp.contains("addconstraint")) {
                    p.paraseAddCS(sql, db);
                    ui->Terminal->append("添加约束成功");
                    Log::writeToLog(db.path, dclFacade->currentSession().username, sql);
                    refreshDBTreeWithState();
                    ui->sqlEdit->clear();
                } else {
                    p.paraseAddCol(sql, db);
                    ui->Terminal->append("添加字段成功");
                    Log::writeToLog(db.path, dclFacade->currentSession().username, sql);
                    refreshDBTreeWithState();
                    ui->sqlEdit->clear();
                }
            }
            if (temp.contains("drop")) {
                if (temp.contains("dropcolumn")) {
                    p.paraseDTableF(sql, db.path, db);
                    ui->Terminal->append("删除字段成功");
                    Log::writeToLog(db.path, dclFacade->currentSession().username, sql);
                    refreshDBTreeWithState();
                    ui->sqlEdit->clear();
                } else {
                    p.paraseDTKEY(sql, db);
                    ui->Terminal->append("删除约束成功");
                    Log::writeToLog(db.path, dclFacade->currentSession().username, sql);
                    refreshDBTreeWithState();
                    ui->sqlEdit->clear();
                }
            }
            if (temp.contains("modify")) {
                p.paraseModifyCol(sql, db);
                ui->Terminal->append("修改成功");
                Log::writeToLog(db.path, dclFacade->currentSession().username, sql);
                refreshDBTreeWithState();
                ui->sqlEdit->clear();
            }
            if (temp.contains("change")) {
                p.paraseChangeCol(sql, db);
                ui->Terminal->append("修改成功");
                Log::writeToLog(db.path, dclFacade->currentSession().username, sql);
                refreshDBTreeWithState();
                ui->sqlEdit->clear();
            }
        } catch (const std::invalid_argument& e) {
            ui->Terminal->append("SQL执行失败：" + QString(e.what()));
            Log::writeToLog(db.path, dclFacade->currentSession().username, QString(e.what()));
        }
    }
    else if (sql.startsWith("DROP TABLE", Qt::CaseInsensitive)) {
        try {
            p.paraseDropTable(sql, db);
            ui->Terminal->append("删除成功");
            Log::writeToLog(db.path, dclFacade->currentSession().username, sql);
            refreshDBTreeWithState();
            ui->sqlEdit->clear();
        } catch (const std::invalid_argument& e) {
            ui->Terminal->append("SQL执行失败：" + QString(e.what()));
            Log::writeToLog(db.path, dclFacade->currentSession().username, QString(e.what()));
        }
    }
    else if (sql.startsWith("DROP DATABASE", Qt::CaseInsensitive)) {
        try {
            p.paraseDropDatabase(sql);
            ui->Terminal->append("删除数据库成功");
            QString spath = PROJECT_ROOT_DIR "/dataDB/sys";
            Log::writeToLog(spath, dclFacade->currentSession().username, sql);
            refreshDBTreeWithState();
            ui->sqlEdit->clear();
        } catch (const std::invalid_argument& e) {
            ui->Terminal->append("SQL执行失败：" + QString(e.what()));
            QString spath = PROJECT_ROOT_DIR "/dataDB/sys";
            Log::writeToLog(spath, dclFacade->currentSession().username, QString(e.what()));
        }
    }
    // DML
    else if (sql.startsWith("INSERT", Qt::CaseInsensitive)) {
        try {
            InsertStatement stmt = p.parseInsert(sql);
            int affected = DML::executeInsert(db, stmt);
            ui->Terminal->append(QString("插入成功，影响 %1 行").arg(affected));
            Log::writeToLog(db.path, dclFacade->currentSession().username, sql);
        } catch (const std::invalid_argument& e) {
            ui->Terminal->append(QString("SQL语句执行失败：%1").arg(e.what()));
            Log::writeToLog(db.path, dclFacade->currentSession().username, QString("SQL语句执行失败：%1").arg(e.what()));
        }
    } else if (sql.startsWith("UPDATE", Qt::CaseInsensitive)) {
        try {
            UpdateStatement stmt = p.parseUpdate(sql);
            int affected = DML::executeUpdate(db, stmt);
            ui->Terminal->append(QString("更新成功，影响 %1 行").arg(affected));
            Log::writeToLog(db.path, dclFacade->currentSession().username, sql);
        } catch (const std::invalid_argument& e) {
            ui->Terminal->append(QString("SQL语句执行失败：").arg(e.what()));
            Log::writeToLog(db.path, dclFacade->currentSession().username, QString("SQL语句执行失败：%1").arg(e.what()));
        }
    } else if (sql.startsWith("DELETE", Qt::CaseInsensitive)) {
        try {
            DeleteStatement stmt = p.parseDelete(sql);
            int affected = DML::executeDelete(db, stmt);
            ui->Terminal->append(QString("删除成功，影响 %1 行").arg(affected));
            Log::writeToLog(db.path, dclFacade->currentSession().username, sql);
        } catch (const std::invalid_argument& e) {
            ui->Terminal->append(QString("SQL语句执行失败：%1").arg(e.what()));
            Log::writeToLog(db.path, dclFacade->currentSession().username, QString("SQL语句执行失败：%1").arg(e.what()));
        }
    }
    // DQL
    else if (sql.startsWith("SELECT", Qt::CaseInsensitive)) {
        try {
            QString result = DQL::executeQuery(db, sql);
            ui->Terminal->append(result);
        } catch (const std::invalid_argument& e) {
            ui->Terminal->append(QString("SQL语句执行失败：%2").arg(e.what()));
        }
    }
}

// ==================== 日志查看 ====================

void MainWindow::loadLogFileList()
{
    ui->logFileList->clear();
    m_dateLogs.clear();

    // 收集所有日志目录
    QStringList logDirs;
    logDirs << QString(PROJECT_ROOT_DIR) + "/dataDB/sys/logs";

    QSet<QString> paths = getAllDbRootPaths();
    for (const auto& p : paths) {
        QDir rootDir(p);
        QFileInfoList dbDirs = rootDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QFileInfo& dbInfo : dbDirs) {
            QString logPath = dbInfo.absoluteFilePath() + "/logs";
            if (QDir(logPath).exists() && !logDirs.contains(logPath))
                logDirs << logPath;
        }
    }

    // 收集所有日志文件，按日期聚合： date → [(dbName, filePath)]
    for (const QString& logDir : logDirs) {
        QDir dir(logDir);
        QString dbName = QFileInfo(logDir).dir().dirName();
        QFileInfoList files = dir.entryInfoList({"*.log"}, QDir::Files, QDir::Name | QDir::Reversed);
        for (const QFileInfo& fi : files) {
            QString date = fi.completeBaseName();  // yyyy-MM-dd
            m_dateLogs[date].append({dbName, fi.absoluteFilePath()});
        }
    }

    // 左侧列表：按日期倒序
    QStringList dates = m_dateLogs.keys();
    std::sort(dates.begin(), dates.end(), std::greater<QString>());

    for (const QString& date : dates) {
        int dbCount = m_dateLogs[date].size();
        // 统计该日期总条目数
        int totalEntries = 0;
        for (const auto& pair : m_dateLogs[date])
            totalEntries += pair.second.length();

        QListWidgetItem *item = new QListWidgetItem(
            QString("%1   (%2个数据库)").arg(date).arg(dbCount));
        item->setData(Qt::UserRole, date);
        item->setToolTip(QString("%1 共 %2 个数据库").arg(date).arg(dbCount));
        ui->logFileList->addItem(item);
    }

    if (ui->logFileList->count() == 0) {
        ui->logContentTree->clear();
        ui->logEntryCountLabel->clear();
    }
}

// 日志条目
struct LogEntry {
    QString time;     // HH:mm:ss
    QString user;
    QString sql;
};

void MainWindow::loadLogContent(const QString& date)
{
    ui->logSearchInput->clear();
    ui->logContentTree->clear();

    if (!m_dateLogs.contains(date)) {
        ui->logContentTree->addTopLevelItem(new QTreeWidgetItem({"无日志"}));
        return;
    }

    const auto& dbFiles = m_dateLogs[date];
    int totalEntries = 0;

    // 遍历每个数据库的该日期日志
    for (const auto& dbPair : dbFiles) {
        const QString& dbName = dbPair.first;
        const QString& filePath = dbPair.second;

        // QMap<小时范围, QList<LogEntry>>
        QMap<QString, QList<LogEntry>> hourMap;

        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) continue;

        QTextStream in(&file);
        in.setEncoding(QStringConverter::Utf8);
        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            if (line.isEmpty()) continue;

            LogEntry entry;
            int endBracket = line.indexOf(']');
            if (endBracket < 0 || !line.startsWith('[')) continue;

            entry.time = line.mid(1, endBracket - 1);

            int dashPos = line.indexOf('-', endBracket);
            if (dashPos < 0) continue;

            int colonPos = line.indexOf(':', dashPos);
            if (colonPos < 0) continue;

            entry.user = line.mid(dashPos + 1, colonPos - dashPos - 1);
            entry.sql = line.mid(colonPos + 1);

            int h = entry.time.left(2).toInt();
            QString hourRange = QString("%1:00~%2:00")
                .arg(h, 2, 10, QChar('0'))
                .arg((h + 1) % 24, 2, 10, QChar('0'));

            hourMap[hourRange].append(entry);
        }
        file.close();

        if (hourMap.isEmpty()) continue;

        // 数据库节点
        int dbTotal = 0;
        for (const auto& list : hourMap) dbTotal += list.size();
        totalEntries += dbTotal;

        QTreeWidgetItem* dbNode = new QTreeWidgetItem(
            {QString("%1  (%2条)").arg(dbName).arg(dbTotal)});
        dbNode->setData(0, Qt::UserRole, "db");
        QFont dbFont = dbNode->font(0);
        dbFont.setBold(true);
        dbFont.setPointSize(11);
        dbNode->setFont(0, dbFont);
        dbNode->setForeground(0, QColor("#2c3e50"));

        // 时间段节点
        for (auto hourIt = hourMap.begin(); hourIt != hourMap.end(); ++hourIt) {
            const QString& hourRange = hourIt.key();
            const QList<LogEntry>& entries = hourIt.value();

            QTreeWidgetItem* hourNode = new QTreeWidgetItem(
                {QString(" %1  (%2条)").arg(hourRange).arg(entries.size())});
            hourNode->setData(0, Qt::UserRole, "hour");
            hourNode->setForeground(0, QColor("#34495e"));

            for (const LogEntry& e : entries) {
                QString display = QString("%1  %2  %3")
                    .arg(e.time, -8)
                    .arg(e.user, -12)
                    .arg(e.sql);
                QTreeWidgetItem* entryNode = new QTreeWidgetItem({display});
                entryNode->setData(0, Qt::UserRole, "entry");
                entryNode->setToolTip(0, e.sql);
                hourNode->addChild(entryNode);
            }
            dbNode->addChild(hourNode);
        }
        ui->logContentTree->addTopLevelItem(dbNode);
        dbNode->setExpanded(true);
    }

    ui->logEntryCountLabel->setText(QString("共 %1 条").arg(totalEntries));
}

void MainWindow::on_btnRefreshLogs_clicked()
{
    loadLogFileList();
}

void MainWindow::onLogFileItemClicked(QListWidgetItem *item)
{
    if (!item) return;
    QString date = item->data(Qt::UserRole).toString();
    loadLogContent(date);
}

void MainWindow::onLogSearchChanged(const QString& keyword)
{
    for (int i = 0; i < ui->logContentTree->topLevelItemCount(); ++i) {
        QTreeWidgetItem* dbNode = ui->logContentTree->topLevelItem(i);
        bool dbMatched = false;

        for (int j = 0; j < dbNode->childCount(); ++j) {
            QTreeWidgetItem* hourNode = dbNode->child(j);
            bool hourMatched = false;

            for (int k = 0; k < hourNode->childCount(); ++k) {
                QTreeWidgetItem* entry = hourNode->child(k);
                bool match = keyword.isEmpty()
                    || entry->text(0).contains(keyword, Qt::CaseInsensitive);
                entry->setHidden(!match);
                if (match) hourMatched = true;
            }
            hourNode->setHidden(!hourMatched);
            if (hourMatched) dbMatched = true;
        }
        dbNode->setHidden(!dbMatched);
    }
}
