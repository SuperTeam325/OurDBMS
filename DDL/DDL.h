#ifndef DDL_H
#define DDL_H

#include <QString>
#include <cstdint>
#include <QException>
#include <stdexcept>
#include <QVector>
#include <QMap>
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

        FieldConstraint()
            : not_null(false),
            default_val(""),
            Primary_key(false),
            Unique_key(false)
        {}
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

        if(field_type==FieldType::INT && length==0) length=4;
        if(field_type==FieldType::FLOAT && length==0) length=8;
        if(field_type==FieldType::CHAR && length==0) length=1;

    }
};

struct Table{
    QString name;
    QVector<Field> fields;

    bool hasField(const QString& Fname){
        for(const auto f:fields){
            if(f.field_name==Fname) return true;
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


    bool hasTable(const QString& tableName) const {
        return tables.contains(tableName);
    }
};

// ========== 新增：函数声明 ==========
// 辅助函数声明：字符串转字段类型
static FieldType parseFieldType(const QString& typeStr);

// 核心函数声明：解析CREATE TABLE SQL，返回Table对象
static Table parseCreateTable(const QString& sql);

};

#endif // DDL_H
