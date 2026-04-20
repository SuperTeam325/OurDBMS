#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "dialog.h"
#include "./ui_dialog.h"
#include "Lexer.h"
#include <QDir>
#include "../DCL/dcl_facade.h"

MainWindow::MainWindow(DCL::DclFacade* facade, QWidget *parent)
    : QMainWindow(parent)
    , dclFacade(facade)
    , ui(new Ui::MainWindow)
{

    ui->setupUi(this);
    if (dclFacade && dclFacade->isLoggedIn()) {
        setWindowTitle("MainWindow - 用户: " + dclFacade->currentSession().username);
    } else {
        setWindowTitle("MainWindow - 未登录");
    }
    displayDB();

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

     if (dclFacade) {
         QString sessionMessage;
         QString sessionError;
         if (dclFacade->tryHandleSessionSql(sql, sessionMessage, sessionError)) {
             if (!sessionError.isEmpty()) {
                 ui->Terminal->append("SQL执行失败：" + sessionError);
             } else {
                 ui->Terminal->append(sessionMessage);
                 if (dclFacade->isLoggedIn()) {
                     setWindowTitle("MainWindow - 用户: " + dclFacade->currentSession().username);
                 } else {
                     setWindowTitle("MainWindow - 未登录");
                 }
             }
             ui->sqlEdit->clear();
             return;
         }
     }

     if (!dclFacade || !dclFacade->isLoggedIn()) {
         ui->Terminal->append("SQL执行失败：未登录");
         return;
     }

     QString authError;
     if (!dclFacade->authorizeSql(sql, authError)) {
         ui->Terminal->append("SQL执行失败：" + authError);
         return;
     }

    //大小写不敏感
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
             ui->Terminal->append("切换成功");
             ui->sqlEdit->clear();

         }catch (const std::invalid_argument& e) {
             ui->Terminal->append("SQL执行失败：" +QString(e.what()));
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
            //刷新显示
             refreshDBTreeWithState();

            ui->sqlEdit->clear();
        }catch (const std::invalid_argument& e) {
            ui->Terminal->append("SQL执行失败：" +QString(e.what()));
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
                ui->Terminal->append("添加成功");
                //刷新显示
                refreshDBTreeWithState();
                ui->sqlEdit->clear();
            }
            else {
                p.paraseAddCol(sql,db);
                ui->Terminal->append("添加成功");
                //刷新显示
                refreshDBTreeWithState();
                ui->sqlEdit->clear();
            }
            //添加约束：
        }
            if(temp.contains("drop")){
                if (temp.contains("dropcolumn")) {
                    p.paraseDTableF(sql,db.path,db);

                    ui->Terminal->append("删除成功");
                    //刷新显示
                     refreshDBTreeWithState();
                    ui->sqlEdit->clear();
                }else{
                    p.paraseDTKEY(sql,db);
                    ui->Terminal->append("删除成功");
                    //刷新显示
                     refreshDBTreeWithState();
                    ui->sqlEdit->clear();
                }
            }
            if(temp.contains("modify")){
                p.paraseModifyCol(sql,db);
                ui->Terminal->append("修改成功");
                //刷新显示
                refreshDBTreeWithState();
                ui->sqlEdit->clear();
            }

       }catch (const std::invalid_argument& e) {
           ui->Terminal->append("SQL执行失败：" +QString(e.what()));
       }
    }
}


void MainWindow::on_SetPath_clicked()
{
    // 1. 创建弹窗对象（父窗口设为this，自动管理内存）
    Dialog *dialog = new Dialog(this);

    // 2. 【关键】设置为非模态，主窗口可操作
    dialog->setModal(false);

    // 3. 显示弹窗
    dialog->show();

    connect(dialog, &QDialog::accepted, this, [=]() {
        QString path = dialog->ui->lineEdit->text();
        qDebug()<<"获取数据库存储路径"<<path;
        DBpath=path;
    });

}

void MainWindow::displayDB()
{
    // 清空原有内容
    ui->treeWidget->clear();

    // 暂定写死根路径
    QString path = "../../dataDB";
    QDir rootDir(path);

    // 获取所有数据库文件夹
    QFileInfoList dbDirList = rootDir.entryInfoList(
        QDir::Dirs | QDir::NoDotAndDotDot
        );

    // 一级节点：用户1
    QTreeWidgetItem *USERItem = new QTreeWidgetItem(ui->treeWidget);
    USERItem->setText(0, "用户1");
    USERItem->setExpanded(true);

    // 二级节点：数据库
    QTreeWidgetItem *dbGroupItem = new QTreeWidgetItem(USERItem);
    dbGroupItem->setText(0, "数据库");


    // 遍历每个数据库
    for (QFileInfo dbInfo : dbDirList)
    {
        // 数据库名
        QTreeWidgetItem *dbItem = new QTreeWidgetItem(dbGroupItem);
        dbItem->setText(0, dbInfo.fileName());


        // 数据库下 表分组
        QTreeWidgetItem *tableGroupItem = new QTreeWidgetItem(dbItem);
        tableGroupItem->setText(0, "表");


        // 进入数据库，获取所有表文件夹
        QDir dbFolder(dbInfo.absoluteFilePath());
        QFileInfoList tableDirList = dbFolder.entryInfoList(
            QDir::Dirs | QDir::NoDotAndDotDot
            );

        // 遍历每个表文件夹
        for (QFileInfo tableInfo : tableDirList)
        {

            // 表节点
            QString tableName = tableInfo.fileName();
            QTreeWidgetItem *tableItem = new QTreeWidgetItem(tableGroupItem);
            tableItem->setText(0, tableName);

            // 拼接 .tbs 文件路径
            // C:\...\test\users\users.tbs
            QString tbsPath = tableInfo.absoluteFilePath() + "/" + tableName + ".tbs";


            // 调用你的函数 → 读取表结构
            DDL::Table table = DDL::loadSchema(tbsPath);

            //列分组
            QTreeWidgetItem *colGroupItem = new QTreeWidgetItem(tableItem);
            colGroupItem->setText(0, "列");



            // 遍历字段 → 生成树形节点
            for (const DDL::Field& f : table.fields)
            {
                // 字段显示：name (type, len)
                QString fieldText = QString("%1 (%2, %3)")
                                        .arg(f.field_name)
                                        .arg(DDL::fieldTypeToString(f.field_type))
                                        .arg(f.length);

                QTreeWidgetItem *fieldItem = new QTreeWidgetItem(colGroupItem);
                fieldItem->setText(0, fieldText);
            }

            //约束分组
            QTreeWidgetItem *ConstGroupItem = new QTreeWidgetItem(tableItem);
            ConstGroupItem->setText(0, "约束");
            QVector<TokenType> CSType={TOKEN_NOT, TOKEN_DEFAULT,TOKEN_PRIMARY,TOKEN_UNIQUE,TOKEN_AUTO_INCREMENT,TOKEN_FOREIGN};
            for (const DDL::Field& f : table.fields)
            {
                // 字段显示：约束名（约束类型）
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
}
// 工具1：保存所有展开节点的路径
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

// 工具2：根据路径恢复展开状态
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
    // 1. 保存当前所有展开节点的路径
    QStringList expandedPaths;
    for (int i = 0; i < ui->treeWidget->topLevelItemCount(); ++i) {
        expandedPaths += saveExpandedPaths(ui->treeWidget->topLevelItem(i), "");
    }

    // 2. 刷新UI（重新生成树形结构）
    displayDB();

    // 3. 恢复展开状态
    for (int i = 0; i < ui->treeWidget->topLevelItemCount(); ++i) {
        restoreExpandedPaths(ui->treeWidget->topLevelItem(i), "", expandedPaths);
    }
}
