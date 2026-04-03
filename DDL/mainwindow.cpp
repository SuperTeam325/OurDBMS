#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "dialog.h"
#include "./ui_dialog.h"
#include "Lexer.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{

    ui->setupUi(this);

   /* DDL::DataBase db = DDL::loadSchema();
    ui->Terminal->append("数据库名：" + db.name);
    ui->Terminal->append("已加载表数量：" + QString::number(db.tables.size()));

    // 遍历打印所有表名（验证是否真的读到）
    for(auto it = db.tables.begin();it!=db.tables.end();it++){
        ui->Terminal->append("→ 表：" + it.key());
    }*/
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_SubmitSQL_clicked()
{
    //加载已有数据库
   /* DDL::DataBase schoolDB = DDL::loadSchema();
    // 如果数据库从未初始化，再设置名字
    if (schoolDB.name.isEmpty()) {
        schoolDB.name = "school";
    }*/

     QString sql=ui->sqlEdit->toPlainText().trimmed();
    //大小写不敏感
     if(sql.startsWith("CREATE DATABASE", Qt::CaseInsensitive)){
         try{

             p.paraseCreateDB(sql,DBpath);

             ui->Terminal->append("数据库创建成功");
             ui->sqlEdit->clear();
         }catch (const std::invalid_argument& e) {
             ui->Terminal->append("SQL执行失败：" +QString(e.what()));
         }
     }
     else if(sql.startsWith("USE", Qt::CaseInsensitive)){
         try{
             //进入的db只拿到路径和名字信息
             p.paraseUSEDB(sql,db);
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
           // Parser p(ts);
            t=p.parseCreateTable(sql,db);

            DDL::writeToDbs(db,t);


           /* ui->Terminal->append("表名:"+t.name);
            for(auto f:t.fields){
                ui->Terminal->append("字段名:"+f.field_name);
                ui->Terminal->append("字段类型:"+DDL::fieldTypeToString(f.field_type));
                ui->Terminal->append("字段长度:"+QString::number(f.length));
                ui->Terminal->append("字段约束:"+f.field_Constraint.toString());
            }*/
            DDL::saveSchema(t,db.path);
            ui->Terminal->append("建表成功");

            db.tables[t.name]=t;




           // 添加新表
           /* schoolDB.tables[t.name] = t;

            // 保存结构 → schema.dbs
            DDL::saveSchema(schoolDB);

            // 初始化空数据 → 表名.dbf
            DDL::saveTableData(t, {});*/
            ui->sqlEdit->clear();
        } catch (const std::invalid_argument& e) {
            ui->Terminal->append("SQL执行失败：" +QString(e.what()));
            }
    }
     try{
         DDL::loadSchema(t,db.path);
     }catch(const std::invalid_argument& e) {
         ui->Terminal->append("SQL执行失败：" +QString(e.what()));
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


