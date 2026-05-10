#ifndef DML_H
#define DML_H

#include "DDL.h"
#include "Index.h"
#include <QString>
#include <QVector>
#include <QMap>

// ==============================================
// 解析后的值结构：区分字面量来源
// ==============================================
struct ParsedValue {
    QString value;    // 实际值内容
    bool quoted;      // 是否来自字符串字面量（单引号）

    ParsedValue() : value(""), quoted(false) {}
    ParsedValue(const QString& v, bool q) : value(v), quoted(q) {}
};

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

    // 加载表数据
    static QVector<QVector<QString>> loadTableRows(const DDL::DataBase& db, const DDL::Table& table);

private:
    // 获取表数据文件路径：db.path/tableName/tableName.tbf
    static QString getTableDataFilePath(const DDL::DataBase& db, const QString& tableName);

    // 保存表数据
    static void saveTableRows(const DDL::DataBase& db, const DDL::Table& table, const QVector<QVector<QString>>& rows);

    // 检查值是否为空（NULL 或空字符串）
    static bool isNullLike(const QString& value);

    // 类型校验：检查值是否符合字段类型（支持区分是否带引号）
    static void validateFieldValue(const DDL::Field& field, const QString& value, bool quoted);

    // 检查主键/唯一约束冲突（支持排除指定行，可选索引加速）
    static bool hasDuplicateKey(const QVector<QVector<QString>>& rows,
                                int fieldIndex, const QString& value,
                                int excludeRow = -1,
                                BPlusTree* index = nullptr,
                                const QString& lookupValue = QString());

    // 获取自增字段的下一个值
    static int getNextAutoIncrement(const QVector<QVector<QString>>& rows, int fieldIndex);

    // 外键约束校验：检查外键字段的值是否在被引用表中存在（可选索引加速）
    static bool validateForeignKey(const DDL::DataBase& db, const DDL::Field& field,
                                   const QString& value, BPlusTree* index = nullptr,
                                   const QString& lookupValue = QString());

    // 处理字段值：应用默认值（如果需要）
    static QString applyDefault(const DDL::Field& field, const QString& value);

    // 统一校验函数：对单个字段值进行完整校验
    static void validateFieldConstraint(
        const DDL::DataBase& db,
        const DDL::Table& table,
        const DDL::Field& field,
        const QString& value,
        bool quoted,
        int currentRowIndex = -1,
        bool isPrimaryKey = false
        );
};

#endif // DML_H
