#include "DML.h"
#include <QFile>
#include <QDataStream>
#include <QDir>
#include <QDebug>

// ==============================================
// 前向声明：检查表是否存在（通过检查 .tbs 文件）
// ==============================================
static bool tableExists(const DDL::DataBase& db, const QString& tableName);

// ==============================================
// 辅助函数：获取表数据文件路径
// ==============================================
QString DML::getTableDataFilePath(const DDL::DataBase& db, const QString& tableName)
{
    return db.path + "/" + tableName + "/" + tableName + ".tbf";
}

// ==============================================
// 辅助函数：字段类型转字符串
// ==============================================
static QString fieldTypeToString(DDL::FieldType type)
{
    switch (type) {
    case DDL::FieldType::INT:    return "INT";
    case DDL::FieldType::FLOAT:  return "FLOAT";
    case DDL::FieldType::CHAR:   return "CHAR";
    case DDL::FieldType::VARCHAR: return "VARCHAR";
    default:                      return "UNKNOWN";
    }
}

// ==============================================
// 辅助函数：加载表数据
// ==============================================
QVector<QVector<QString>> DML::loadTableRows(const DDL::DataBase& db, const DDL::Table& table)
{
    QVector<QVector<QString>> rows;
    QString fileName = getTableDataFilePath(db, table.name);
    QFile file(fileName);

    qDebug() << "\n========================================";
    qDebug() << "         .tbf 文件读取信息            ";
    qDebug() << "========================================";
    qDebug() << "[基本信息]";
    qDebug() << "  文件路径:" << fileName;
    qDebug() << "  表名:" << table.name;

    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "[状态] 文件不存在，返回空数据";
        qDebug() << "========================================\n";
        return rows;
    }

    qDebug() << "[文件信息]";
    qDebug() << "  文件大小:" << file.size() << "字节";

    QDataStream in(&file);
    in.setVersion(QDataStream::Qt_5_15);

    int rowCount;
    in >> rowCount;

    int fieldCount = table.fields.size();
    qDebug() << "[结构信息]";
    qDebug() << "  行数 (rowCount):" << rowCount;
    qDebug() << "  字段数 (fieldCount):" << fieldCount;

    qDebug() << "[字段定义]";
    for (int i = 0; i < fieldCount; i++) {
        const DDL::Field& f = table.fields[i];
        qDebug() << "  [" << i << "]" << f.field_name
                 << "(" << fieldTypeToString(f.field_type)
                 << ", 长度:" << f.length << ")";
    }

    qDebug() << "[数据内容]";
    for (int i = 0; i < rowCount; i++) {
        QVector<QString> row;
        qDebug() << "  [行" << i << "]";
        for (int j = 0; j < fieldCount; j++) {
            QString val;
            in >> val;
            row.append(val);
            qDebug() << "    [" << j << "]" << table.fields[j].field_name
                     << " = \"" << val << "\"";
        }
        rows.append(row);
    }

    qDebug() << "========================================\n";

    file.close();
    return rows;
}

// ==============================================
// 辅助函数：保存表数据
// ==============================================
void DML::saveTableRows(const DDL::DataBase& db, const DDL::Table& table, const QVector<QVector<QString>>& rows)
{
    QString tableDir = db.path + "/" + table.name;
    QDir dir;
    if (!dir.exists(tableDir)) {
        dir.mkpath(tableDir);
    }

    QString fileName = getTableDataFilePath(db, table.name);
    QFile file(fileName);

    qDebug() << "\n========================================";
    qDebug() << "         .tbf 文件写入信息            ";
    qDebug() << "========================================";
    qDebug() << "[基本信息]";
    qDebug() << "  文件路径:" << fileName;
    qDebug() << "  表名:" << table.name;
    qDebug() << "  写入行数:" << rows.size();
    qDebug() << "  字段数:" << table.fields.size();

    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        throw std::invalid_argument("无法打开数据文件进行写入");
    }

    QDataStream out(&file);
    out.setVersion(QDataStream::Qt_5_15);

    qDebug() << "[写入数据]";
    qDebug() << "  [文件头] rowCount =" << rows.size();

    out << (int)rows.size();
    for (int i = 0; i < rows.size(); i++) {
        qDebug() << "  [行" << i << "]";
        for (int j = 0; j < rows[i].size(); j++) {
            out << rows[i][j];
            qDebug() << "    [" << j << "]" << table.fields[j].field_name
                     << " = \"" << rows[i][j] << "\"";
        }
    }

    qDebug() << "[写入完成]";
    qDebug() << "  文件大小:" << file.size() << "字节";
    qDebug() << "========================================\n";

    file.close();
}

// ==============================================
// 辅助函数：类型校验
// ==============================================
void DML::validateFieldValue(const DDL::Field& field, const QString& value)
{
    // 空字符串允许（除非字段有NOT NULL约束，这会在后面检查）
    if (value.isEmpty()) return;

    switch (field.field_type) {
    case DDL::FieldType::INT: {
        bool ok;
        value.toInt(&ok);
        if (!ok) {
            throw std::invalid_argument(QString("字段 %1 类型应为 INT").arg(field.field_name).toStdString());
        }
        break;
    }
    case DDL::FieldType::FLOAT: {
        bool ok;
        value.toDouble(&ok);
        if (!ok) {
            throw std::invalid_argument(QString("字段 %1 类型应为 FLOAT").arg(field.field_name).toStdString());
        }
        break;
    }
    case DDL::FieldType::CHAR:
    case DDL::FieldType::VARCHAR: {
        if (field.length > 0 && value.length() > field.length) {
            throw std::invalid_argument(QString("字段 %1 超过长度限制 (%2)").arg(field.field_name).arg(field.length).toStdString());
        }
        break;
    }
    default:
        break;
    }
}

// ==============================================
// 辅助函数：检查主键/唯一约束冲突
// ==============================================
bool DML::hasDuplicateKey(const QVector<QVector<QString>>& rows, int fieldIndex, const QString& value, int excludeRow)
{
    for (int i = 0; i < rows.size(); i++) {
        if (i == excludeRow) continue;
        if (rows[i].size() > fieldIndex && rows[i][fieldIndex] == value) {
            return true;
        }
    }
    return false;
}

// ==============================================
// 辅助函数：外键约束校验
// 检查外键字段的值是否在被引用表中存在
// ==============================================
bool DML::validateForeignKey(const DDL::DataBase& db, const DDL::Field& field, const QString& value)
{
    // 如果字段不是外键，或者值为空（允许NULL除非有NOT NULL约束），直接返回true
    if (!field.field_Constraint.Foreign_key) {
        return true;
    }
    if (value.isEmpty()) {
        return true; // 外键允许NULL值（除非另有约束）
    }

    const QString& refTableName = field.field_Constraint.ref_table;
    const QString& refFieldName = field.field_Constraint.ref_field;

    // 检查被引用表是否存在
    if (!tableExists(db, refTableName)) {
        qDebug() << "外键引用表不存在:" << refTableName;
        return false;
    }

    // 加载被引用表的结构
    QString refSchemaPath = db.path + "/" + refTableName + "/" + refTableName + ".tbs";
    DDL::Table refTable = DDL::loadSchema(refSchemaPath);

    // 获取被引用字段的索引
    int refFieldIndex = refTable.getFieldIndex(refFieldName);
    if (refFieldIndex < 0) {
        qDebug() << "外键引用字段不存在:" << refFieldName;
        return false;
    }

    // 加载被引用表的数据
    QVector<QVector<QString>> refRows = loadTableRows(db, refTable);

    // 检查值是否在被引用表中存在
    for (const auto& row : refRows) {
        if (row.size() > refFieldIndex && row[refFieldIndex] == value) {
            return true; // 找到匹配的值，外键有效
        }
    }

    return false; // 未找到匹配的值，外键约束违反
}

// ==============================================
// 辅助函数：获取自增字段的下一个值
// ==============================================
int DML::getNextAutoIncrement(const QVector<QVector<QString>>& rows, int fieldIndex)
{
    int maxVal = 0;
    for (const auto& row : rows) {
        if (row.size() > fieldIndex) {
            bool ok;
            int val = row[fieldIndex].toInt(&ok);
            if (ok && val > maxVal) {
                maxVal = val;
            }
        }
    }
    return maxVal + 1;
}

// ==============================================
// 辅助函数：构建完整的行数据
// ==============================================
QVector<QString> DML::buildFullRow(const DDL::Table& table, const QVector<QString>& columns, const QVector<QString>& values)
{
    QVector<QString> fullRow(table.fields.size(), "");

    // 如果没有指定列，按顺序映射
    if (columns.isEmpty()) {
        if (values.size() != table.fields.size()) {
            throw std::invalid_argument(QString("值数量不匹配：期望 %1 个，实际 %2 个")
                .arg(table.fields.size()).arg(values.size()).toStdString());
        }
        for (int i = 0; i < values.size(); i++) {
            fullRow[i] = values[i];
        }
    } else {
        // 按列名映射
        if (values.size() != columns.size()) {
            throw std::invalid_argument(QString("值数量与列名数量不匹配").toStdString());
        }

        for (int i = 0; i < columns.size(); i++) {
            int idx = table.getFieldIndex(columns[i]);
            if (idx < 0) {
                throw std::invalid_argument(QString("字段 %1 不存在").arg(columns[i]).toStdString());
            }
            fullRow[idx] = values[i];
        }

        // 处理未指定的字段：默认值、自增、空值
        for (int i = 0; i < table.fields.size(); i++) {
            if (fullRow[i].isEmpty()) {
                const DDL::Field& field = table.fields[i];
                const DDL::FieldConstraint& con = field.field_Constraint;

                // AUTO_INCREMENT
                if (con.Auto_increasement) {
                    // 需要先读取已有数据来计算下一个值，这里先用占位符
                    // 实际在 executeInsert 中处理
                    continue;
                }
                // DEFAULT
                if (!con.default_val.isEmpty()) {
                    fullRow[i] = con.default_val;
                }
                // NOT NULL 会在后面检查
            }
        }
    }

    return fullRow;
}

// ==============================================
// 辅助函数：检查表是否存在（通过检查 .tbs 文件）
// ==============================================
static bool tableExists(const DDL::DataBase& db, const QString& tableName)
{
    QString schemaPath = db.path + "/" + tableName + "/" + tableName + ".tbs";
    QFile file(schemaPath);
    return file.exists();
}

// ==============================================
// INSERT 执行（支持单行和多行 VALUES）
// ==============================================
int DML::executeInsert(const DDL::DataBase& db, const InsertStatement& stmt)
{
    if (db.path.isEmpty()) {
        throw std::invalid_argument("未指定数据库");
    }

    if (!tableExists(db, stmt.tableName)) {
        throw std::invalid_argument(QString("表 %1 不存在").arg(stmt.tableName).toStdString());
    }

    // 加载表结构
    QString schemaPath = db.path + "/" + stmt.tableName + "/" + stmt.tableName + ".tbs";
    DDL::Table table = DDL::loadSchema(schemaPath);

    // 加载现有数据
    QVector<QVector<QString>> rows = loadTableRows(db, table);
    int insertedCount = 0;

    // 遍历每一行 VALUES (...),(...),...
    for (const QVector<QString>& rowValues : stmt.rows) {
        // 构建完整行数据
        QVector<QString> fullRow = buildFullRow(table, stmt.columns, rowValues);

        // 处理自增字段
        for (int i = 0; i < table.fields.size(); i++) {
            const DDL::Field& field = table.fields[i];
            if (field.field_Constraint.Auto_increasement && fullRow[i].isEmpty()) {
                fullRow[i] = QString::number(getNextAutoIncrement(rows, i));
            }
        }

        // 逐字段校验
        for (int i = 0; i < table.fields.size(); i++) {
            const DDL::Field& field = table.fields[i];
            const QString& value = fullRow[i];

            // 类型校验
            validateFieldValue(field, value);

            // NOT NULL 校验
            if (field.field_Constraint.not_null && value.isEmpty()) {
                throw std::invalid_argument(QString("字段 %1 不能为空").arg(field.field_name).toStdString());
            }

            // PRIMARY KEY 校验
            if (field.field_Constraint.Primary_key) {
                if (value.isEmpty()) {
                    throw std::invalid_argument(QString("主键字段 %1 不能为空").arg(field.field_name).toStdString());
                }
                if (hasDuplicateKey(rows, i, value)) {
                    throw std::invalid_argument(QString("主键 %1 重复").arg(value).toStdString());
                }
            }

            // UNIQUE 校验
            if (field.field_Constraint.Unique_key) {
                if (!value.isEmpty() && hasDuplicateKey(rows, i, value)) {
                    throw std::invalid_argument(QString("唯一约束字段 %1 值 %2 重复").arg(field.field_name).arg(value).toStdString());
                }
            }

            // FOREIGN KEY 校验
            if (field.field_Constraint.Foreign_key) {
                if (!validateForeignKey(db, field, value)) {
                    throw std::invalid_argument(QString("外键约束违反：字段 %1 的值 %2 在被引用表 %3 的字段 %4 中不存在")
                        .arg(field.field_name)
                        .arg(value)
                        .arg(field.field_Constraint.ref_table)
                        .arg(field.field_Constraint.ref_field)
                        .toStdString());
                }
            }
        }

        // 追加新行
        rows.append(fullRow);
        insertedCount++;
    }

    // 保存
    saveTableRows(db, table, rows);

    // === 调试：验证 .tbf 数据是否正确写入 ===
    QVector<QVector<QString>> verifyRows = DDL::loadTableData(table, db.path);
    qDebug() << "=== 当前表数据 ===";
    if (verifyRows.isEmpty()) {
        qDebug() << "(empty)";
    } else {
        for (int i = 0; i < verifyRows.size(); i++) {
            QString line;
            for (int j = 0; j < table.fields.size(); j++) {
                line += table.fields[j].field_name + ":" + verifyRows[i][j] + " ";
            }
            qDebug() << line;
        }
    }
    qDebug() << "===================";

    return 1;
}

// ==============================================
// UPDATE 执行
// ==============================================
int DML::executeUpdate(const DDL::DataBase& db, const UpdateStatement& stmt)
{
    if (db.path.isEmpty()) {
        throw std::invalid_argument("未指定数据库");
    }

    if (!tableExists(db, stmt.tableName)) {
        throw std::invalid_argument(QString("表 %1 不存在").arg(stmt.tableName).toStdString());
    }

    // 加载表结构
    QString schemaPath = db.path + "/" + stmt.tableName + "/" + stmt.tableName + ".tbs";
    DDL::Table table = DDL::loadSchema(schemaPath);

    // 加载现有数据
    QVector<QVector<QString>> rows = loadTableRows(db, table);

    // 检查 SET 字段是否存在
    for (auto it = stmt.setMap.begin(); it != stmt.setMap.end(); ++it) {
        if (!table.hasField(it.key())) {
            throw std::invalid_argument(QString("字段 %1 不存在").arg(it.key()).toStdString());
        }
    }

    // 检查 WHERE 字段是否存在
    int whereIdx = table.getFieldIndex(stmt.whereColumn);
    if (whereIdx < 0) {
        throw std::invalid_argument(QString("WHERE 字段 %1 不存在").arg(stmt.whereColumn).toStdString());
    }

    int affected = 0;

    for (int rowIdx = 0; rowIdx < rows.size(); rowIdx++) {
        QVector<QString>& row = rows[rowIdx];

        // 检查是否满足 WHERE 条件
        if (row[whereIdx] != stmt.whereValue) {
            continue;
        }

        // 复制原行用于校验
        QVector<QString> originalRow = row;

        // 应用 SET
        for (auto it = stmt.setMap.begin(); it != stmt.setMap.end(); ++it) {
            int colIdx = table.getFieldIndex(it.key());
            row[colIdx] = it.value();
        }

        // 逐字段校验
        for (auto it = stmt.setMap.begin(); it != stmt.setMap.end(); ++it) {
            int colIdx = table.getFieldIndex(it.key());
            const DDL::Field& field = table.fields[colIdx];
            const QString& value = row[colIdx];

            // 类型校验
            validateFieldValue(field, value);

            // NOT NULL 校验
            if (field.field_Constraint.not_null && value.isEmpty()) {
                row = originalRow; // 恢复原值
                throw std::invalid_argument(QString("字段 %1 不能为空").arg(field.field_name).toStdString());
            }

            // PRIMARY KEY 校验
            if (field.field_Constraint.Primary_key) {
                // 主键不能为空
                if (value.isEmpty()) {
                    row = originalRow;
                    throw std::invalid_argument(QString("主键 %1 不能为空").arg(field.field_name).toStdString());
                }
                // 主键不能重复
                if (hasDuplicateKey(rows, colIdx, value, rowIdx)) {
                    row = originalRow;
                    throw std::invalid_argument(QString("主键 %1 重复").arg(value).toStdString());
                }
            }

            // UNIQUE 校验
            if (field.field_Constraint.Unique_key) {
                if (!value.isEmpty() && hasDuplicateKey(rows, colIdx, value, rowIdx)) {
                    row = originalRow;
                    throw std::invalid_argument(QString("唯一约束字段 %1 值 %2 重复").arg(field.field_name).arg(value).toStdString());
                }
            }

            // FOREIGN KEY 校验
            if (field.field_Constraint.Foreign_key) {
                if (!validateForeignKey(db, field, value)) {
                    row = originalRow; // 恢复原值
                    throw std::invalid_argument(QString("外键约束违反：字段 %1 的值 %2 在被引用表 %3 的字段 %4 中不存在")
                        .arg(field.field_name)
                        .arg(value)
                        .arg(field.field_Constraint.ref_table)
                        .arg(field.field_Constraint.ref_field)
                        .toStdString());
                }
            }
        }

        affected++;
    }

    if (affected > 0) {
        saveTableRows(db, table, rows);

        // === 调试：验证 .tbf 数据是否正确写入 ===
        QVector<QVector<QString>> verifyRows = DDL::loadTableData(table, db.path);
        qDebug() << "=== 当前表数据 ===";
        if (verifyRows.isEmpty()) {
            qDebug() << "(empty)";
        } else {
            for (int i = 0; i < verifyRows.size(); i++) {
                QString line;
                for (int j = 0; j < table.fields.size(); j++) {
                    line += table.fields[j].field_name + ":" + verifyRows[i][j] + " ";
                }
                qDebug() << line;
            }
        }
        qDebug() << "===================";
    }

    return affected;
}

// ==============================================
// DELETE 执行
// ==============================================
int DML::executeDelete(const DDL::DataBase& db, const DeleteStatement& stmt)
{
    if (db.path.isEmpty()) {
        throw std::invalid_argument("未指定数据库");
    }

    if (!tableExists(db, stmt.tableName)) {
        throw std::invalid_argument(QString("表 %1 不存在").arg(stmt.tableName).toStdString());
    }

    // 加载表结构
    QString schemaPath = db.path + "/" + stmt.tableName + "/" + stmt.tableName + ".tbs";
    DDL::Table table = DDL::loadSchema(schemaPath);

    // 加载现有数据
    QVector<QVector<QString>> rows = loadTableRows(db, table);

    // 检查 WHERE 字段是否存在
    int whereIdx = table.getFieldIndex(stmt.whereColumn);
    if (whereIdx < 0) {
        throw std::invalid_argument(QString("WHERE 字段 %1 不存在").arg(stmt.whereColumn).toStdString());
    }

    // 物理删除：保留不满足条件的行
    int originalCount = rows.size();
    for (int i = rows.size() - 1; i >= 0; i--) {
        if (rows[i][whereIdx] == stmt.whereValue) {
            rows.removeAt(i);
        }
    }

    int deleted = originalCount - rows.size();

    if (deleted > 0) {
        saveTableRows(db, table, rows);

        // === 调试：验证 .tbf 数据是否正确写入 ===
        QVector<QVector<QString>> verifyRows = DDL::loadTableData(table, db.path);
        qDebug() << "=== 当前表数据 ===";
        if (verifyRows.isEmpty()) {
            qDebug() << "(empty)";
        } else {
            for (int i = 0; i < verifyRows.size(); i++) {
                QString line;
                for (int j = 0; j < table.fields.size(); j++) {
                    line += table.fields[j].field_name + ":" + verifyRows[i][j] + " ";
                }
                qDebug() << line;
            }
        }
        qDebug() << "===================";
    }

    return deleted;
}

// ==============================================
// SELECT * 执行（全表查询）
// ==============================================
QString DML::executeSelect(const DDL::DataBase& db, const SelectStatement& stmt)
{
    if (db.path.isEmpty()) {
        throw std::invalid_argument("未指定数据库");
    }

    if (!tableExists(db, stmt.tableName)) {
        throw std::invalid_argument(QString("表 %1 不存在").arg(stmt.tableName).toStdString());
    }

    // 加载表结构
    QString schemaPath = db.path + "/" + stmt.tableName + "/" + stmt.tableName + ".tbs";
    DDL::Table table = DDL::loadSchema(schemaPath);

    // 使用 DDL::loadTableData 读取数据
    QVector<QVector<QString>> rows = DDL::loadTableData(table, db.path);

    // 构建输出结果
    QString result;

    // 表头
    for (int i = 0; i < table.fields.size(); i++) {
        if (i > 0) result += " | ";
        result += table.fields[i].field_name;
    }
    result += "\n";

    // 分隔线
    for (int i = 0; i < table.fields.size(); i++) {
        if (i > 0) result += "-+-";
        result += QString().fill('-', table.fields[i].field_name.length());
    }
    result += "\n";

    // 数据行
    if (rows.isEmpty()) {
        result += "(empty)";
    } else {
        for (int i = 0; i < rows.size(); i++) {
            for (int j = 0; j < rows[i].size(); j++) {
                if (j > 0) result += " | ";
                result += rows[i][j];
            }
            if (i < rows.size() - 1) {
                result += "\n";
            }
        }
    }

    return result;
}
