#include "mainwindow.h"
#include "./ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_SubmitSQL_clicked()
{
    DDL::DataBase schoolDB;
    schoolDB.name = "school";

    QString sql=ui->sqlEdit->toPlainText().trimmed();
    //大小写不敏感
    if(sql.startsWith("CREATE TABLE", Qt::CaseInsensitive)){
        try {
            // 解析SQL，直接生成Table对象
            DDL::Table studentTable = DDL::parseCreateTable(sql);

            // 插入到数据库里，和你之前手动创建的效果完全一样！
            schoolDB.tables.insert(studentTable.name, studentTable);

            ui->Terminal->append("表格创建成功!\n");
            qDebug() << "表名：" << studentTable.name;
            qDebug() << "是否包含 'name' 字段：" << studentTable.hasField("name");
            qDebug() << "'score' 字段的索引：" << studentTable.getFieldIndex("score");
            qDebug() << "是否包含 'student' 表：" << schoolDB.hasTable("student");

        } catch (const std::invalid_argument& e) {
            ui->Terminal->append("SQL执行失败：" +QString(e.what()));
            }
    }

}

