#include "DDL.h"
#include <QFile>
#include <QDataStream>
#include <QDebug>
#include <QDir>

//保存表结构（字段名，类型，约束）
/*void DDL::saveSchema(const DataBase& db){
    QFile file("schema.dbs");
    if(!file.open(QIODevice::WriteOnly)) return;

    QDataStream out(&file);

    out<<db.name;
    out<<(int)db.tables.size();

    //表字段信息
    for(QMap<QString,Table>::const_iterator it=db.tables.begin();it!=db.tables.end();it++){
        out<<it->name;
        out<<(int)it->fields.size();
        for(auto f:it->fields){
            out<<f.field_name;
            out<<(int)f.field_type; //枚举类型转为（int）才能正确写入文件
            out<<(int)f.length;
            out<<f.field_Constraint.not_null;
            out << f.field_Constraint.default_val;
            out << f.field_Constraint.Primary_key;
            out << f.field_Constraint.Unique_key;
        }
    }
    file.close();
}*/
void DDL::saveSchema(DDL::Table& table,QString& path){

    if(path.isEmpty()){
          throw::std::invalid_argument("未指定数据库");
    }

    //创建表文件夹
    QString TablePath=path+"/"+table.name;
    QDir dir;
    if (!dir.exists(TablePath)) {
        dir.mkpath(TablePath);
    }
    //对于表的结构文件
    QFile file(TablePath+"/"+table.name+".tbs");
    //这里是创建的写入，不追加也没啥
    if(!file.open(QIODevice::WriteOnly | QIODevice::Append)) return;
    QDataStream out(&file);

    //out<<db.name;
    //out<<(int)db.tables.size();

    //表字段信息
    //for(QMap<QString,Table>::const_iterator it=db.tables.begin();it!=db.tables.end();it++){
        out<<table.name;
        out<<(int)table.fields.size();
        for(auto f:table.fields){
            out<<f.field_name;
            out<<(int)f.field_type; //枚举类型转为（int）才能正确写入文件
            out<<(int)f.length;
            out<<f.field_Constraint.not_null;
            out << f.field_Constraint.default_val;
            out << f.field_Constraint.Primary_key;
            out << f.field_Constraint.Unique_key;
        }
   // }
    file.close();
}


//读取表结构
DDL::DataBase DDL::loadSchema()
{
    DataBase db;
    QFile file("schema.dbs");
    if(!file.open(QIODevice::ReadOnly)) return db;

    QDataStream in(&file);

    in>>db.name;
    int tableCount;
    in >> tableCount;
    qDebug() << "读取表数量：" << tableCount;

    for(int i=0;i<tableCount;i++){
        Table table;
        in >> table.name;
        int fieldsCount;
        in>>fieldsCount;

        for(int j=0;j<fieldsCount;j++){
            FieldConstraint fc;
            QString name;
            int type;
            int len=0;

            in >> name;          
            in >> type;
            in >> len;
            in >> fc.not_null;
            in >> fc.default_val;
            in >> fc.Primary_key;
            in >> fc.Unique_key;

            Field f(name,(FieldType)type,(uint16_t)len,fc);
            table.fields.append(f);
            qDebug()<<"字段名:"+f.field_name;
            qDebug()<<"字段类型:"+DDL::fieldTypeToString(f.field_type);
            qDebug()<<"字段长度:"+QString::number(f.length);
            qDebug()<<"字段约束:"+f.field_Constraint.toString();
        }
        db.tables[table.name]=table;
    }
    file.close();
    return db;
}

//写入DBS文件
void DDL::writeToDbs(DataBase& db,Table& t){
    QString path=db.path+"/"+db.name+".dbs";
    QFile f(path);
    //追加模式
    if (!f.open(QIODevice::Append | QIODevice::Append)) {
        qDebug()<<"写入DBS失败";
        return;
    }
    qDebug()<<"创建的dbs名字"+path;

    QDataStream out(&f);
    out<<t.name;
    f.close();
}

// ==============================================
// 保存表数据 → 表名.dbf
// ==============================================
void DDL::saveTableData(const DDL::Table &table, const QVector<QVector<QString>> &rows)
{
    QString fileName = table.name + ".dbf";
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly)) return;

    QDataStream out(&file);
    out << (int)rows.size();

    for (const auto &row : rows) {
        for (const QString &val : row) {
            out << val;
        }
    }

    file.close();
}

// ==============================================
// 加载表数据 ← 表名.dbf
// ==============================================
QVector<QVector<QString>> DDL::loadTableData(const DDL::Table &table)
{
    QVector<QVector<QString>> rows;
    QString fileName = table.name + ".dbf";
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) return rows;

    QDataStream in(&file);
    int rowCount;
    in >> rowCount;

    int fieldCount = table.fields.size();
    for (int i = 0; i < rowCount; i++) {
        QVector<QString> row;
        for (int j = 0; j < fieldCount; j++) {
            QString val;
            in >> val;
            row.append(val);
        }
        rows.append(row);
    }

    file.close();
    return rows;
}
