#ifndef DML_H
#define DML_H

#include "DDL.h"
#include <QString>
#include <QVector>
#include <QMap>

// DML 语句解析结果结构
struct InsertStatement {
    QString tableName;
    QVector<QString> columns;   // 可为空，表示按表字段顺序插入
    QVector< QVector<QString> > rows;  // 支持多行 VALUES：每行一个 QVector
};

struct UpdateStatement {
    QString tableName;
    QMap<QString, QString> setMap;  // 字段名 -> 新值
    QString whereColumn;
    QString whereValue;
};

struct DeleteStatement {
    QString tableName;
    QString whereColumn;
    QString whereValue;
};

struct SelectStatement {
    QString tableName;
};

class DML {
public:
    // INSERT 执行
    static int executeInsert(const DDL::DataBase& db, const InsertStatement& stmt);

    // UPDATE 执行
    static int executeUpdate(const DDL::DataBase& db, const UpdateStatement& stmt);

    // DELETE 执行
    static int executeDelete(const DDL::DataBase& db, const DeleteStatement& stmt);

    // SELECT * 执行（全表查询）
    static QString executeSelect(const DDL::DataBase& db, const SelectStatement& stmt);

private:
    // 获取表数据文件路径：db.path/tableName/tableName.tbf
    static QString getTableDataFilePath(const DDL::DataBase& db, const QString& tableName);

    // 加载表数据
    static QVector<QVector<QString>> loadTableRows(const DDL::DataBase& db, const DDL::Table& table);

    // 保存表数据
    static void saveTableRows(const DDL::DataBase& db, const DDL::Table& table, const QVector<QVector<QString>>& rows);

    // 类型校验：检查值是否符合字段类型
    static void validateFieldValue(const DDL::Field& field, const QString& value);

    // 检查主键/唯一约束冲突（插入时）
    static bool hasDuplicateKey(const QVector<QVector<QString>>& rows, int fieldIndex, const QString& value, int excludeRow = -1);

    // 获取自增字段的下一个值
    static int getNextAutoIncrement(const QVector<QVector<QString>>& rows, int fieldIndex);

    // 外键约束校验：检查外键字段的值是否在被引用表中存在
    static bool validateForeignKey(const DDL::DataBase& db, const DDL::Field& field, const QString& value);

    // 构建完整的行数据（处理默认值、自增等）
    static QVector<QString> buildFullRow(const DDL::Table& table, const QVector<QString>& columns, const QVector<QString>& values);
};

#endif // DML_H
