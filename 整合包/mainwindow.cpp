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
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include "DML.h"
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
    if (dclFacade && dclFacade->isLoggedIn()) {
        setWindowTitle("MainWindow - 用户: " + dclFacade->currentSession().username);
        ui->currentUserTabel->setText("当前用户:"+dclFacade->currentSession().username);
    } else {
        setWindowTitle("MainWindow - 未登录");
         ui->currentUserTabel->setText("未登录用户");

    }
    displayDB();
    // ========== 开启右键菜单 ==========
    ui->treeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->treeWidget, &QTreeWidget::customContextMenuRequested,
            this, &MainWindow::onTreeRightClicked);

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_SubmitSQL_clicked()
{
     QString sql=ui->sqlEdit->toPlainText().trimmed();
    if (sql.isEmpty()) {
        return;
    }
    //用户功能
    //  优先处理dcl语句
    if (dclFacade) {
        QString dclMessage;
        QString dclError;
        if (dclFacade->tryHandleSessionSql(sql, dclMessage, dclError)) {
            if (!dclError.isEmpty()) {
                ui->Terminal->append("SQL执行失败：" + dclError);
            } else {
                ui->Terminal->append(dclMessage);
                if (dclFacade->isLoggedIn()) {
                    setWindowTitle("MainWindow - 用户: " + dclFacade->currentSession().username);
                    ui->currentUserTabel->setText("当前用户:"+dclFacade->currentSession().username);
                } else {
                    setWindowTitle("MainWindow - 未登录");
                    ui->currentUserTabel->setText("未登录用户");
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

    //DDL
     if(sql.startsWith("CREATE DATABASE", Qt::CaseInsensitive)){
         try{

             p.paraseCreateDB(sql,DBpath);

             ui->Terminal->append("数据库创建成功");
             //刷新显示
              refreshDBTreeWithState();
             ui->sqlEdit->clear();
         }catch (const std::invalid_argument& e) {
             ui->Terminal->append("SQL执行失败：" +QString(e.what()));
         }
     }
     else if(sql.startsWith("USE", Qt::CaseInsensitive)){
         try{
             //进入的db只拿到路径和名字信息
             p.paraseUSEDB(sql,db);
             dclFacade->setCurrentDatabase(db.name);
             ui->Terminal->append(QString("切换成功,当前数据库：%1").arg(db.name));
             //写入日志
             Log::writeToLog(db.path,dclFacade->currentSession().username,sql);

             ui->sqlEdit->clear();

         }catch (const std::invalid_argument& e) {
             ui->Terminal->append("SQL执行失败：" +QString(e.what()));
             //写入日志
             Log::writeToLog(db.path,dclFacade->currentSession().username,QString(e.what()));
         }

     }
     else if(sql.startsWith("CREATE TABLE", Qt::CaseInsensitive)){
        try {
            Lexer l;
            QList<Token> ts=l.ReadSQL(sql);
            for(auto t:ts){
                qDebug()<<"字段名:"<<t.text<<"类型:"<<t.type;

            }

            t=p.parseCreateTable(sql,db);

            DDL::writeToDbs(db,t);

            DDL::saveSchema(t,db.path);
            ui->Terminal->append("建表成功");
            //写入日志
            Log::writeToLog(db.path,dclFacade->currentSession().username,sql);

            //刷新显示
             refreshDBTreeWithState();

            ui->sqlEdit->clear();
        }catch (const std::invalid_argument& e) {
            ui->Terminal->append("SQL执行失败：" +QString(e.what()));
            //写入日志
            Log::writeToLog(db.path,dclFacade->currentSession().username,QString(e.what()));
            }
    }else if(sql.startsWith("ALTER TABLE", Qt::CaseInsensitive)){

       try{
            QString lowerSql = sql.toLower(); // 统一转小写
            QString temp = sql.toLower();
            temp.replace("\n", "");
            temp.replace("\r", "");
            temp.replace(" ", "");
        if(temp.contains("add")){
            if (temp.contains("addconstraint")){


                p.paraseAddCS(sql,db);
                ui->Terminal->append("添加约束成功");
                Log::writeToLog(db.path,dclFacade->currentSession().username,sql);
                //刷新显示
                refreshDBTreeWithState();
                ui->sqlEdit->clear();
            }
            else {
                p.paraseAddCol(sql,db);
                ui->Terminal->append("添加字段成功");
                Log::writeToLog(db.path,dclFacade->currentSession().username,sql);
                //刷新显示
                refreshDBTreeWithState();
                ui->sqlEdit->clear();
            }
            //添加约束：
          }
            if(temp.contains("drop")){
                if (temp.contains("dropcolumn")) {

                    p.paraseDTableF(sql,db.path,db);

                    ui->Terminal->append("删除字段成功");
                    //写入日志
                    Log::writeToLog(db.path,dclFacade->currentSession().username,sql);
                    //刷新显示
                     refreshDBTreeWithState();
                    ui->sqlEdit->clear();
                }else{
                    p.paraseDTKEY(sql,db);
                    ui->Terminal->append("删除约束成功");
                    //写入日志
                    Log::writeToLog(db.path,dclFacade->currentSession().username,sql);
                    //刷新显示
                     refreshDBTreeWithState();
                    ui->sqlEdit->clear();
                }

            }
            if(temp.contains("modify")){
                p.paraseModifyCol(sql,db);
                ui->Terminal->append("修改成功");
                //写入日志
                Log::writeToLog(db.path,dclFacade->currentSession().username,sql);
                //刷新显示
                refreshDBTreeWithState();
                ui->sqlEdit->clear();
            }

            if(temp.contains("change")){
                p.paraseChangeCol(sql,db);
                ui->Terminal->append("修改成功");
                //写入日志
                Log::writeToLog(db.path,dclFacade->currentSession().username,sql);
                //刷新显示
                refreshDBTreeWithState();
                ui->sqlEdit->clear();
            }

       }catch (const std::invalid_argument& e) {
           ui->Terminal->append("SQL执行失败：" +QString(e.what()));
           //写入日志
           Log::writeToLog(db.path,dclFacade->currentSession().username,QString(e.what()));
       }
    }else if(sql.startsWith("DROP TABLE", Qt::CaseInsensitive)){

        try{
            p.paraseDropTable(sql,db);
            ui->Terminal->append("删除成功");
            //写入日志
            Log::writeToLog(db.path,dclFacade->currentSession().username,sql);
            //刷新显示
            refreshDBTreeWithState();
            ui->sqlEdit->clear();
        }catch (const std::invalid_argument& e) {
            ui->Terminal->append("SQL执行失败：" +QString(e.what()));
            //写入日志
            Log::writeToLog(db.path,dclFacade->currentSession().username,QString(e.what()));
        }


    }else if(sql.startsWith("DROP DATABASE", Qt::CaseInsensitive)){
        try{
            p.paraseDropDatabase(sql);
            ui->Terminal->append("删除数据库成功");
             //写入日志
             QString spath=PROJECT_ROOT_DIR"/dataDB/sys";
            Log::writeToLog(spath,dclFacade->currentSession().username,sql);
            //刷新显示
            refreshDBTreeWithState();
            ui->sqlEdit->clear();
        }catch (const std::invalid_argument& e) {
            ui->Terminal->append("SQL执行失败：" +QString(e.what()));
            //写入日志,删除数据库的日志写入系统库里
            QString spath=PROJECT_ROOT_DIR"/dataDB/sys";
            Log::writeToLog(spath,dclFacade->currentSession().username,QString(e.what()));
        }

    }
    //===========
    //DML模块
    //==========
    else if (sql.startsWith("INSERT", Qt::CaseInsensitive)) {
        try {
            InsertStatement stmt = p.parseInsert(sql);
            int affected = DML::executeInsert(db, stmt);
            ui->Terminal->append(QString("插入成功，影响 %1 行").arg(affected));
            //写入日志
            Log::writeToLog(db.path,dclFacade->currentSession().username,sql);
        } catch (const std::invalid_argument& e) {
            ui->Terminal->append(QString("SQL语句执行失败：%1").arg(e.what()));
            //写入日志
            Log::writeToLog(db.path,dclFacade->currentSession().username,QString("SQL语句执行失败：%1").arg(e.what()));
        }
    } else if (sql.startsWith("UPDATE", Qt::CaseInsensitive)) {
        try {
            UpdateStatement stmt = p.parseUpdate(sql);
            int affected = DML::executeUpdate(db, stmt);
            ui->Terminal->append(QString("更新成功，影响 %1 行").arg(affected));
            //写入日志
            Log::writeToLog(db.path,dclFacade->currentSession().username,sql);
        } catch (const std::invalid_argument& e) {
            ui->Terminal->append(QString( "SQL语句执行失败：").arg(e.what()));
            //写入日志
            Log::writeToLog(db.path,dclFacade->currentSession().username,QString("SQL语句执行失败：%1").arg(e.what()));

        }
    } else if (sql.startsWith("DELETE", Qt::CaseInsensitive)) {
        try {
            DeleteStatement stmt = p.parseDelete(sql);
            int affected = DML::executeDelete(db, stmt);
            ui->Terminal->append(QString("删除成功，影响 %1 行").arg(affected));
            //写入日志
            Log::writeToLog(db.path,dclFacade->currentSession().username,sql);
        } catch (const std::invalid_argument& e) {
            ui->Terminal->append(QString("SQL语句执行失败：%1").arg(e.what()));
            //写入日志
            Log::writeToLog(db.path,dclFacade->currentSession().username,QString("SQL语句执行失败：%1").arg(e.what()));

        }
    }
    //DQL
    else if (sql.startsWith("SELECT", Qt::CaseInsensitive)) {
        try {
            QString result = DQL::executeQuery(db, sql);
            ui->Terminal->append(result);

        } catch (const std::invalid_argument& e) {
            ui->Terminal->append(QString("SQL语句执行失败：%2").arg(e.what()));
        }
    }



    /*try{
        QString path="C:/Users/21495/Desktop/DBMS测试/test/u/u.tbs";
         DDL::loadSchema(path);

     }catch(const std::invalid_argument& e) {
         ui->Terminal->append("SQL执行失败：" +QString(e.what()));
     }*/


}


void MainWindow::on_SetPath_clicked()
{
    //  创建弹窗对象（父窗口设为this，自动管理内存）
    Dialog *dialog = new Dialog(this);

    // 非模态
    dialog->setModal(false);

    dialog->show();

    connect(dialog, &QDialog::accepted, this, [=]() {
        QString path = dialog->ui->lineEdit->text();
        qDebug()<<"获取数据库存储路径"<<path;
        DBpath=path;
    });

}


//获取所有数据库的根目录
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

    // 存储不重复的根目录
    QSet<QString> rootPaths;

    // 遍历所有数据库路径，自动提取根目录
    for (auto it = obj.begin(); it != obj.end(); ++it) {
        QString dbPath = it.value().toString();
        QString rootPath = QFileInfo(dbPath).absolutePath();
        rootPaths.insert(rootPath);
    }

    return rootPaths;
}



//后续路径通过读取db_config.jso文件灵活识别路径
void MainWindow::displayDB()
{
    ui->treeWidget->clear();
    //改进：写个循环读取数据库信息文件获取每个数据库的目录依次存到dbDirList,写一个QFileInfoLiSt数组

    QSet<QString> paths=getAllDbRootPaths();
    QFileInfoList AllDbDirList;
    for(auto const p:paths){
         QDir rootDir(p);
         QFileInfoList dbDirList = rootDir.entryInfoList(
            QDir::Dirs | QDir::NoDotAndDotDot
            );
         AllDbDirList.append(dbDirList);
    }


    // 创建顶级分组
    QTreeWidgetItem *normalDbGroup = new QTreeWidgetItem(ui->treeWidget);
    normalDbGroup->setText(0, "数据库");

    QTreeWidgetItem *systemDbGroup = new QTreeWidgetItem(ui->treeWidget);
    systemDbGroup->setText(0, "系统数据库");

    QTreeWidgetItem *UserGroup = new QTreeWidgetItem(ui->treeWidget);
    UserGroup->setText(0, "用户");


    // 遍历所有数据库文件夹
    for (QFileInfo dbInfo : AllDbDirList)
    {
        QString dbName = dbInfo.fileName();
        QTreeWidgetItem *targetGroup;

        // 核心逻辑：名字是 sys → 系统数据库，否则普通数据库
        if (dbName == "sys") {
            targetGroup = systemDbGroup;
        } else {
            targetGroup = normalDbGroup;
        }

        // 数据库节点
        QTreeWidgetItem *dbItem = new QTreeWidgetItem(targetGroup);
        dbItem->setText(0, dbName);

        // 表分组
        QTreeWidgetItem *tableGroupItem = new QTreeWidgetItem(dbItem);
        tableGroupItem->setText(0, "表");

        // 加载表
        QDir dbFolder(dbInfo.absoluteFilePath());
        QFileInfoList tableDirList = dbFolder.entryInfoList(
            QDir::Dirs | QDir::NoDotAndDotDot
            );

        for (QFileInfo tableInfo : tableDirList)
        {
            QString tableName = tableInfo.fileName();



            if(tableInfo.fileName()=="logs"){
                continue;
            }

            QTreeWidgetItem *tableItem = new QTreeWidgetItem(tableGroupItem);
            tableItem->setText(0, tableName);

            QString tbsPath = tableInfo.absoluteFilePath() + "/" + tableName + ".tbs";
            DDL::Table table = DDL::loadSchema(tbsPath);

            // 列
            QTreeWidgetItem *colGroupItem = new QTreeWidgetItem(tableItem);
            colGroupItem->setText(0, "列");
            for (const DDL::Field& f : table.fields)
            {
                QString fieldText = QString("%1 (%2, %3)")
                .arg(f.field_name)
                    .arg(DDL::fieldTypeToString(f.field_type))
                    .arg(f.length);
                QTreeWidgetItem *fieldItem = new QTreeWidgetItem(colGroupItem);
                fieldItem->setText(0, fieldText);
            }

            // 约束
            QTreeWidgetItem *ConstGroupItem = new QTreeWidgetItem(tableItem);
            ConstGroupItem->setText(0, "约束");
            QVector<TokenType> CSType={TOKEN_NOT,TOKEN_DEFAULT,TOKEN_PRIMARY,TOKEN_UNIQUE,TOKEN_AUTO_INCREMENT,TOKEN_FOREIGN};
            for (const DDL::Field& f : table.fields)
            {
                for(auto cst:CSType){
                    if(!f.field_Constraint.Const_Name[cst].isEmpty()){
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
    //加载用户信息
    QString path = PROJECT_ROOT_DIR "/dataDB"; //sys固定路径
    QVector<QVector<QString>> users = DDL::loadTableData(userReposity.usersTable(),  path+"/sys");
    QVector<QVector<QString>> permission=DDL::loadTableData(userPermission.permissionsTable(), path+"/sys");

    for(int i=0;i<users.size();++i){

        QTreeWidgetItem *NameItem = new QTreeWidgetItem(UserGroup);
        NameItem->setText(0, users[i][0]);
        QTreeWidgetItem *PItem = new QTreeWidgetItem(NameItem);
        PItem->setText(0, "授权");

        if(users[i][3]=="1"){
            QTreeWidgetItem *item = new QTreeWidgetItem(PItem);
            item->setText(0,"ALL PRIVILEGES");
        }
        for(int j=0;j<permission.size();++j){
            if(users[i][0]==permission[j][0]){
                QTreeWidgetItem *item1 = new QTreeWidgetItem(PItem);
                item1->setText(0,QString(permission[j][3]+"(%1,%2)").arg(permission[j][1]).arg(permission[j][2]));
            }
        }
    }

}




// 保存所有展开节点的路径
QStringList MainWindow::saveExpandedPaths(QTreeWidgetItem *item, const QString &parentPath)
{
    QStringList paths;
    if (!item) return paths;

    // 当前节点的完整路径
    QString currentPath = parentPath.isEmpty() ? item->text(0) : parentPath + "/" + item->text(0);

    // 如果节点是展开的，保存路径
    if (item->isExpanded()) {
        paths.append(currentPath);
    }

    // 递归遍历所有子节点
    for (int i = 0; i < item->childCount(); ++i) {
        paths += saveExpandedPaths(item->child(i), currentPath);
    }
    return paths;
}

//根据路径恢复展开状态
void MainWindow::restoreExpandedPaths(QTreeWidgetItem *item, const QString &parentPath, const QStringList &paths)
{
    if (!item) return;

    QString currentPath = parentPath.isEmpty() ? item->text(0) : parentPath + "/" + item->text(0);

    // 如果路径在保存列表中，展开节点
    if (paths.contains(currentPath)) {
        item->setExpanded(true);
    }

    // 递归恢复子节点
    for (int i = 0; i < item->childCount(); ++i) {
        restoreExpandedPaths(item->child(i), currentPath, paths);
    }
}
void MainWindow::refreshDBTreeWithState()
{
    // 保存当前所有展开节点的路径
    QStringList expandedPaths;
    for (int i = 0; i < ui->treeWidget->topLevelItemCount(); ++i) {
        expandedPaths += saveExpandedPaths(ui->treeWidget->topLevelItem(i), "");
    }

    // 刷新UI（重新生成树形结构）
    displayDB();

    // 恢复展开状态
    for (int i = 0; i < ui->treeWidget->topLevelItemCount(); ++i) {
        restoreExpandedPaths(ui->treeWidget->topLevelItem(i), "", expandedPaths);
    }
}

// 右键菜单

void MainWindow::onTreeRightClicked(const QPoint &pos)
{
    QTreeWidgetItem *item = ui->treeWidget->itemAt(pos);
    if (!item) return;

    QMenu menu(this);


    // 判断你点的是什么节点
    QString now = item->text(0);
    QTreeWidgetItem *parent = item->parent();
    QString pText = parent ? parent->text(0) : "";

    bool isDatabaseNode = (pText == "数据库");          //  test、dd 这种
    bool isTableNode    = (pText == "表");              //  enp、users 这种
    bool isColOrConstNode = (pText == "列" || pText == "约束");

    qDebug()<<item->text(0);

    // 菜单内容
    if (isDatabaseNode) {
        // 点的是 数据库
        if(item->text(0)!="sys"){
         menu.addAction("新建表", this, &MainWindow::createTableMenu);
        }
        //管理员有权限删除数据库
        if(dclFacade->currentSession().isAdmin){
            menu.addAction("删除数据库", this, &MainWindow::deleteDatabase);
        }
        menu.addSeparator();
        menu.addAction("刷新", this, &MainWindow::refreshTree);
    }
    else if (isTableNode) {

        menu.addAction("查看表", this, &MainWindow::viewTableMenu);
        menu.addAction("查看数据", this, &MainWindow::viewTableDataMenu);

         if(item->parent()->parent()->text(0)!="sys"){
           menu.addAction("修改表结构", this, &MainWindow::modifyTableMenu);
           menu.addAction("删除表", this, &MainWindow::deleteTableMenu);
         }
        menu.addSeparator();
        menu.addAction("刷新", this, &MainWindow::refreshTree);
    }
    else if (isColOrConstNode) {
        // 点的是 字段/约束
        menu.addAction("查看详情");
        menu.addSeparator();
        menu.addAction("刷新", this, &MainWindow::refreshTree);
    }
    else {
        // 其他
        menu.addAction("刷新", this, &MainWindow::refreshTree);
    }

    // 显示菜单
    menu.exec(ui->treeWidget->viewport()->mapToGlobal(pos));
}

// 刷新
void MainWindow::refreshTree()
{
    refreshDBTreeWithState();
}

// 查看表
void MainWindow::viewTableMenu()
{
    QTreeWidgetItem *item = ui->treeWidget->currentItem();
    if (!item) return;

    // 获取表信息
    QString tableName = item->text(0);
    QString dbName = item->parent()->parent()->text(0);

    QString dbPath=p.getDbPathByName(dbName);
    QString tbsPath = dbPath+ "/" + tableName + "/" + tableName + ".tbs";
    qDebug()<<"dbPath:"<<dbPath;
    DDL::Table table = DDL::loadSchema(tbsPath);

    // 获取第3页
    QWidget *page = ui->stackedWidget->widget(2);

    // 清空旧内容
    if (page->layout() != nullptr) {
        QLayoutItem *child;
        while ((child = page->layout()->takeAt(0)) != nullptr) {
            delete child->widget();
            delete child;
        }
        delete page->layout();
    }

    // 主布局
    QVBoxLayout *mainLayout = new QVBoxLayout(page);
    mainLayout->setContentsMargins(20, 20, 20, 30); //调整边距，按钮整体上移
    mainLayout->setSpacing(15); //按钮离表格更近

    // 表格
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

    // 填充数据
    QVector<TokenType> CSType = {TOKEN_NOT,TOKEN_DEFAULT,TOKEN_PRIMARY,TOKEN_UNIQUE,TOKEN_AUTO_INCREMENT,TOKEN_FOREIGN};
    for (const DDL::Field &f : table.fields) {
        int r = tw->rowCount();
        tw->insertRow(r);
        tw->setItem(r,0,new QTableWidgetItem(f.field_name));
        tw->setItem(r,1,new QTableWidgetItem(DDL::fieldTypeToString(f.field_type)));
        tw->setItem(r,2,new QTableWidgetItem(QString::number(f.length)));

        QString cons;
        for(auto c : CSType){
            if(!f.field_Constraint.Const_Name[c].isEmpty())
                cons += f.field_Constraint.Const_Name[c] + "(" + f.field_Constraint.toString(c) + ")\n";
        }
        tw->setItem(r,3,new QTableWidgetItem(cons.trimmed()));

        for(int c=0;c<4;c++)
            tw->item(r,c)->setTextAlignment(Qt::AlignCenter);
    }


    QPushButton *btnBack = new QPushButton("返回主页");
    btnBack->setMinimumWidth(200);  // 更宽
    btnBack->setStyleSheet(R"(
        QPushButton {
            font-size: 12pt;
            padding: 12px 30px;
        }
    )");

    connect(btnBack, &QPushButton::clicked, this, [=]() {
        ui->stackedWidget->setCurrentIndex(0);
    });

    // 添加到布局
    mainLayout->addWidget(tw);
    mainLayout->addWidget(btnBack, 0, Qt::AlignCenter); // 居中

    ui->stackedWidget->setCurrentIndex(2);
}


//删除数据库
void MainWindow::deleteDatabase(){



    // 打开文件
    QFile file("db_config.json");
    if (!file.open(QIODevice::ReadWrite | QIODevice::Text)) {
        throw::std::invalid_argument("数据库文件出错");
    }

    //解析 JSON
    QByteArray data = file.readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        file.close();
    }

    QJsonObject obj = doc.object();
    file.close();

    QTreeWidgetItem *item = ui->treeWidget->currentItem();

    QString dbName=item->text(0);

    auto btn = QMessageBox::question(this, "删除", "确定删除：" + dbName + "？");

    if(btn == QMessageBox::Yes){

        if (!obj.contains(dbName)) {
            ui->Terminal->append("错误：这不是一个有效的数据库！");
            return;
        }

        QString path=obj[dbName].toString();


        obj.remove(dbName);
        //删除文件夹
        QDir dbDir(path);
        if (dbDir.exists()) {
            dbDir.removeRecursively(); // 强制删除整个文件夹
        }
        //重新写回文件
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
            throw std::invalid_argument("写入配置文件失败");
        }

        doc.setObject(obj);
        file.seek(0);            // 回到文件开头
        file.write(doc.toJson());// 写入新内容
        file.close();
    }

    refreshTree();

}


// 新建表（待实现）
void MainWindow::createTableMenu()
{
    QTreeWidgetItem *item = ui->treeWidget->currentItem();
    if(!item) return;

    QString dbName = item->text(0);
    ui->Terminal->append("右键 → 新建表（数据库：" + dbName + "）");

    // 你可以在这里打开建表窗口
    // 建完后调用 refreshTree();
}

// 删除表
void MainWindow::deleteTableMenu()
{
    QTreeWidgetItem *item = ui->treeWidget->currentItem();
    if(!item) return;

    QString tableName = item->text(0);
    auto btn = QMessageBox::question(this, "删除", "确定删除表：" + tableName + "？");

    if(btn == QMessageBox::Yes){

        QTreeWidgetItem *item = ui->treeWidget->currentItem();
        if (!item) return;

        QString tableName = item->text(0);
        QString dbName = item->parent()->parent()->text(0);

        QString dbPath=p.getDbPathByName(dbName);
        QString tbPath = dbPath+ "/" + tableName;
        QString dbsPath=dbPath+"/"+dbName+".dbs";
        QVector<QString> tableNames;


        //收集已存表名
        //重置
        tableNames.clear();
        QStringList tames=DDL::readFromDbs(dbsPath);
        if(!tames.empty()){
            for(QString name:tames){
                tableNames.append(name);
            }
        }

        //删除表文件夹
        QDir dir(tbPath);
        dir.removeRecursively();

        //移除.dbs文件里的表名
        for(int i=0 ;i<tableNames.size();i++){
            if(tableNames[i]==tableName){
                tableNames.remove(i);
                break;
            }
        }
        QFile f(dbPath+"/"+dbName+".dbs");
        //追加模式
        if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            qDebug()<<"写入DBS失败";
            return;
        }
        QDataStream out(&f);
        for(QString name:tableNames){
            qDebug()<<"重新写入的表名:"<<name;
            out<<name;
        }
        f.close();


        ui->Terminal->append("表 " + tableName + " 已删除");
        refreshTree();
    }
}

// 查看表数据
void MainWindow::viewTableDataMenu()
{
    QTreeWidgetItem *item = ui->treeWidget->currentItem();
    if (!item) return;

    // 获取表信息 + 数据库信息
    QString tableName = item->text(0);
    QString dbName = item->parent()->parent()->text(0);
    QString dbPath = p.getDbPathByName(dbName);

    //  加载表结构
    QString tbsPath = dbPath + "/" + tableName + "/" + tableName + ".tbs";
    DDL::Table table = DDL::loadSchema(tbsPath);

    // 构造数据库对象（给loadTableRows用）
    DDL::DataBase db;
    db.name = dbName;
    db.path = dbPath;

    //读取数据文件
    QVector<QVector<QString>> tableData = DML::loadTableRows(db, table);


    // stackedWidget 第1页
    QWidget *page = ui->stackedWidget->widget(1);

    // 清空旧布局控件
    if (page->layout() != nullptr) {
        QLayoutItem *child;
        while ((child = page->layout()->takeAt(0)) != nullptr) {
            delete child->widget();
            delete child;
        }
        delete page->layout();
    }

    // 主布局
    QVBoxLayout *mainLayout = new QVBoxLayout(page);
    mainLayout->setContentsMargins(20, 20, 20, 30);
    mainLayout->setSpacing(15);


    // 创建数据表格
    QTableWidget *tw = new QTableWidget;
    int columnCount = table.fields.size();    // 列数 = 字段个数
    int rowCount = tableData.size();          // 行数 = 数据行数

    tw->setColumnCount(columnCount);
    tw->setRowCount(rowCount);

    // 设置表头（字段名）
    QStringList headers;
    for (const DDL::Field& f : table.fields) {
        headers << f.field_name;
    }
    tw->setHorizontalHeaderLabels(headers);

    // 表格样式（和你原来的完全一致）
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

    //  填充表数据
    for (int row = 0; row < tableData.size(); row++) {
        const QVector<QString>& rowData = tableData[row];
        for (int col = 0; col < rowData.size(); col++) {
            tw->setItem(row, col, new QTableWidgetItem(rowData[col]));
            tw->item(row, col)->setTextAlignment(Qt::AlignCenter);
        }
    }

    // 空数据提示
    if (tableData.isEmpty()) {
        tw->setRowCount(1);
        tw->setItem(0, 0, new QTableWidgetItem("(表数据为空)"));
        tw->item(0, 0)->setTextAlignment(Qt::AlignCenter);
    }


    // 返回按钮
    QPushButton *btnBack = new QPushButton("返回主页");
    btnBack->setMinimumWidth(200);
    btnBack->setStyleSheet(R"(
        QPushButton {
            font-size: 12pt;
            padding: 12px 30px;
        }
    )");

    connect(btnBack, &QPushButton::clicked, this, [=]() {
        ui->stackedWidget->setCurrentIndex(0);
    });

    // 添加到布局
    mainLayout->addWidget(tw);
    mainLayout->addWidget(btnBack, 0, Qt::AlignCenter);

    ui->stackedWidget->setCurrentIndex(1);
}
// 修改表（待实现）
void MainWindow::modifyTableMenu()
{
    QTreeWidgetItem *item = ui->treeWidget->currentItem();
    if(!item) return;

    QString tableName = item->text(0);
    ui->Terminal->append("右键 → 修改表结构：" + tableName);

    // 打开修改表窗口
    // 修改完调用 refreshTree();
}



