#ifndef DDL_H
#define DDL_H

#include <QString>
#include <cstdint>
#include <QException>
#include <stdexcept>
#include <QVector>
#include <QMap>
#include <QObject>
#include "Lexer.h"
class DDL{
public:
    enum class FieldType{
        INT,
        CHAR,
        VARCHAR,
        FLOAT,
        UNKNOWN
};

struct FieldConstraint{

    bool not_null;
    QString default_val;
    bool Primary_key;
    bool Unique_key;
    bool Auto_increasement;
    bool Foreign_key;

    QMap<TokenType,QString> Const_Name;

    FieldConstraint()
        : not_null(false),
        default_val(""),
        Primary_key(false),
        Unique_key(false),
        Auto_increasement(false),
        Foreign_key(false)
    {}

    QString toString() const {
        QStringList cons;

        if (Primary_key)   cons << "PRIMARY KEY";
        if (not_null)      cons << "NOT NULL";
        if (Unique_key)    cons << "UNIQUE";
        if(Auto_increasement) cons<<"AUTO_INCREMENT";
        if(Foreign_key) cons<<"FOREIGN KEY";
        if (!default_val.isEmpty())
            cons << "DEFAULT " + default_val;

        return cons.join(" ");
    }

    QString toString(TokenType t) const {
        QStringList cons;

        if (t==TOKEN_PRIMARY)   cons << "PRIMARY KEY";
        if (t==TOKEN_NOT)      cons << "NOT NULL";
        if (t==TOKEN_UNIQUE)    cons << "UNIQUE";
        if(t==TOKEN_AUTO_INCREMENT) cons<<"AUTO_INCREMENT";
        if(t==TOKEN_FOREIGN) cons<<"FOREIGN KEY";
        if (!default_val.isEmpty())
            cons << "DEFAULT " + default_val;

        return cons.join(" ");
    }

};
struct Field{
    QString field_name;
    FieldType field_type;
    uint16_t length;
    FieldConstraint field_Constraint;

    Field(QString name, FieldType type, uint16_t l=0, FieldConstraint c={})
        : field_name(name), field_type(type),length(l),field_Constraint(c){

        if(name.isEmpty()){
            throw std::invalid_argument(
                "字段名不能为空"
                );
        }

        if(field_type==FieldType::INT) length=4;
        if(field_type==FieldType::FLOAT ) length=8;
        //else length=l;

    }
};

// 表级约束（例如 UNIQUE (email)）
struct TableConstraint {
    QString name;
    QString type; // UNIQUE / PRIMARY KEY
    QList<QString> columns;
};

struct Table{
    QString name;
    QVector<Field> fields;
    QList<TableConstraint> constraints; // 表级约束

    // 表选项
    QString charset;
    QString collate;

    bool hasField(const QString& Fname){
        for(const auto f:fields){
            if(f.field_name==Fname) return true;
        }
        return false;
    }

    bool hasPK(){
        for(const auto f:fields){
            if(f.field_Constraint.Primary_key) return true;
        }
        return false;
    }

    //获取字段索引
    int getFieldIndex(const QString& Fname) const{
        for(size_t i=0;i<fields.size();i++){
            if(Fname==fields[i].field_name) return (int)i;
        }
        return -1;
    }
};

struct DataBase{
    QString name;
    QMap<QString,Table> tables;
    QString path;


    bool hasTable(const QString& tableName) const {
        return tables.contains(tableName);
    }
};

// 字符串转字段类型
static FieldType parseFieldType(const QString& typeStr);

//解析CREATE TABLE SQL，返回Table对象
//static Table parseCreateTable(const QString& sql);


//辅助函数：字段类型 → 字符串
static QString fieldTypeToString(FieldType type) {
    switch (type) {
    case FieldType::INT: return "INT";
    case FieldType::CHAR: return "CHAR";
    case FieldType::VARCHAR: return "VARCHAR";
    case FieldType::FLOAT: return "FLOAT";
    default: return "UNKNOWN";
    }
}


// 持久化：保存所有表结构到 schema.dbs
static void saveSchema(DDL::Table&,QString&);
//写入DBS文件
static void writeToDbs(DataBase&,Table&);
//读取DBS文件
static QStringList readFromDbs(QString &);

// 加载：从 schema.dbs 读取所有表结构
static Table loadSchema(const QString& path);

// 保存表数据到 表名.dbf
static void saveTableData(const Table &table, const QVector<QVector<QString>> &rows);

// 加载表数据从 表名.dbf
static QVector<QVector<QString>> loadTableData(const Table &table);
};

#endif // DDL_H
