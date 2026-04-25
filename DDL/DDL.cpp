#include "DDL.h"
#include <QFile>
#include <QDataStream>
#include <QDebug>
#include <QDir>

//保存表结构（字段名，类型，约束）
void DDL::saveSchema(DDL::Table& table,QString& path){

    if(path.isEmpty()){
          throw::std::invalid_argument("未指定数据库");
    }

    //创建表文件夹
    QString TablePath=path+"/"+table.name;
    QDir dir;
    if (!dir.exists(TablePath)) { //# 创建表文件夹，保存
        dir.mkpath(TablePath);
    }
    //对于表的结构文件
    QFile file(TablePath+"/"+table.name+".tbs");
    //这里是创建的写入，不追加也没啥
    if(!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) return;
    QDataStream out(&file);
    out.setVersion(QDataStream::Qt_5_15);

    out<<table.name;
    out<<(int)table.fields.size();
    qDebug() << "保存字段数量：" << table.fields.size();

    QString empty_name="";
    //表字段信息
    //for(QMap<QString,Table>::const_iterator it=db.tables.begin();it!=db.tables.end();it++){
        for(auto f:table.fields){
            out<<f.field_name;
            out<<(int)f.field_type; //枚举类型转为（int）才能正确写入文件
            out<<(int)f.length;

            //NOT NULL
            out << f.field_Constraint.not_null;
            if (f.field_Constraint.not_null){
                if(f.field_Constraint.Const_Name[TOKEN_NOT].isEmpty()){
                    out << "__not_null_" + table.name + "_" + f.field_name; //# 写入约束名
                }else{
                    out<<f.field_Constraint.Const_Name[TOKEN_NOT];
                }
            }else{
                out << empty_name;
            }


            // DEFAULT
            out << f.field_Constraint.default_val;
            if (!f.field_Constraint.default_val.isEmpty()){
                if(f.field_Constraint.Const_Name[TOKEN_DEFAULT].isEmpty()){
                     out << "__default_" + table.name + "_" + f.field_name;
                }else{
                    out<<f.field_Constraint.Const_Name[TOKEN_DEFAULT];
                }
            }else{
                  out << empty_name;
            }

            // PRIMARY KEY
            out << f.field_Constraint.Primary_key;
            if (f.field_Constraint.Primary_key){
                if(f.field_Constraint.Const_Name[TOKEN_PRIMARY].isEmpty()){
                    out << "__pk_" + table.name + "_" + f.field_name;

                }else{
                    out<<f.field_Constraint.Const_Name[TOKEN_PRIMARY];
                }
            }else{
                  out << empty_name;
            }

            // UNIQUE
            out << f.field_Constraint.Unique_key;
            if (f.field_Constraint.Unique_key){
                if(f.field_Constraint.Const_Name[TOKEN_UNIQUE].isEmpty()){
                    out << "__unique_" + table.name + "_" + f.field_name;

                }else{
                    out<<f.field_Constraint.Const_Name[TOKEN_UNIQUE];
                }
            }else{
                  out << empty_name;
            }

            // AUTO_INCREMENT
            out << f.field_Constraint.Auto_increasement;
            if (f.field_Constraint.Auto_increasement){
                if(f.field_Constraint.Const_Name[TOKEN_AUTO_INCREMENT].isEmpty()){
                   out << "__auto_" + table.name + "_" + f.field_name;

                }else{
                    out<<f.field_Constraint.Const_Name[TOKEN_AUTO_INCREMENT];
                }
            }else{
                 out << empty_name;
            }
            //FOREIGN KEY
            out<<f.field_Constraint.Foreign_key;
            if (f.field_Constraint.Foreign_key){
                if(f.field_Constraint.Const_Name[TOKEN_FOREIGN].isEmpty()){
                    out << "__Foreign_" + table.name + "_" + f.field_name;

                }else{
                    out<<f.field_Constraint.Const_Name[TOKEN_FOREIGN];
                }
            }else{
                out << empty_name;
            }

        }

   // }
    file.close();
}


//读取表结构
DDL::Table DDL::loadSchema(const QString& path)
{
    qDebug()<<"当前读取文件"<<path;

    QString TablePath=path;
    Table table{};
    QFile file(TablePath);
    if(!file.open(QIODevice::ReadOnly)) {
        qDebug()<<"文件打开失败"<<file.fileName();
        return table;
    }
    QDataStream in(&file);
     in.setVersion(QDataStream::Qt_5_15);

    QVector<TokenType> keyType={TOKEN_NOT, TOKEN_DEFAULT,TOKEN_PRIMARY,TOKEN_UNIQUE,TOKEN_AUTO_INCREMENT,TOKEN_FOREIGN};

        in >> table.name;
        int fieldsCount;
        in>>fieldsCount;

        qDebug()<<"表名:"+table.name;
        qDebug()<<"字段个数"+QString::number(fieldsCount);

        for(int j=0;j<fieldsCount;j++){
            FieldConstraint fc;
            QString name;
            int type;
            int len=0;

            in >> name;
            in >> type;
            in >> len;

            in >> fc.not_null;
            in>> fc.Const_Name[TOKEN_NOT];

            in >> fc.default_val;
            in>> fc.Const_Name[TOKEN_DEFAULT];

            in >> fc.Primary_key;
            in>> fc.Const_Name[TOKEN_PRIMARY];

            in >> fc.Unique_key;
            in>> fc.Const_Name[TOKEN_UNIQUE];

            in >> fc.Auto_increasement;
            in >> fc.Const_Name[TOKEN_AUTO_INCREMENT];

            in >> fc.Foreign_key;
            in >> fc.Const_Name[TOKEN_FOREIGN];

            Field f(name,(FieldType)type,(uint16_t)len,fc);
            table.fields.append(f);

            qDebug()<<"字段名:"+f.field_name;
            qDebug()<<"字段类型:"+DDL::fieldTypeToString(f.field_type);
            qDebug()<<"字段长度:"+QString::number(f.length);
            qDebug()<<"字段约束:"+f.field_Constraint.toString();

            for(const auto& kt:keyType){
                if(!f.field_Constraint.Const_Name[kt].isEmpty()){
                    qDebug()<<"字段约束名:"+f.field_Constraint.Const_Name[kt];
                }
            }

        }
      //  db.tables[table.name]=table;
    file.close();
    return table;
}

//写入DBS文件
void DDL::writeToDbs(DataBase& db,Table& t){
    QString path=db.path+"/"+db.name+".dbs";
    QFile f(path);
    //追加模式
    if (!f.open(QIODevice::Append)) {
        qDebug()<<"写入DBS失败";
        return;
    }
    qDebug()<<"创建的dbs名字"+path;

    QDataStream out(&f);
    out<<t.name;
    f.close();
}
//读取DBS文件
QStringList DDL::readFromDbs(QString& path){
    QFile f(path);
    QStringList list{};
    if(!f.open(QIODevice::ReadOnly)){
        qDebug("读取.dbs失败");
        return list;
    }
    QDataStream in(&f);
    QString tN;
    while(!in.atEnd()){
        in>> tN;
        list.append(tN);
    }
    return list;
}

// ==============================================
// 保存表数据 → 表名.dbf
// ==============================================
void DDL::saveTableData(const DDL::Table &table, const QVector<QVector<QString>> &rows, const QString& dbPath)
{
    QString fileName;
    if (dbPath.isEmpty()) {
        fileName = table.name + ".dbf";
    } else {
        const QString tablePath = dbPath + "/" + table.name;
        QDir dir;
        if (!dir.exists(tablePath)) {
            dir.mkpath(tablePath);
        }
        fileName = tablePath + "/" + table.name + ".dbf";
    }

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
QVector<QVector<QString>> DDL::loadTableData(const DDL::Table &table, const QString& dbPath)
{
    QVector<QVector<QString>> rows;
    QString fileName;
    if (dbPath.isEmpty()) {
        fileName = table.name + ".dbf";
    } else {
        fileName = dbPath + "/" + table.name + "/" + table.name + ".dbf";
    }

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
