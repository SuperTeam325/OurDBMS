#include "DML.h"
#include <QFile>
#include <QDataStream>
#include <QDir>
#include <QDebug>

static bool tableExists(const DDL::DataBase& db, const QString& tableName);

static bool nullLike(const QString& value)
{
    QString trimmed = value.trimmed();
    return trimmed.isEmpty() || trimmed.compare("NULL", Qt::CaseInsensitive) == 0;
}

static bool isQuotedValue(const QString& value)
{
    return value.size() >= 2 && value.startsWith("'") && value.endsWith("'");
}

static QString unquoteValue(const QString& value)
{
    if (isQuotedValue(value)) {
        return value.mid(1, value.size() - 2);
    }
    return value;
}

static bool hasDefaultValue(const DDL::Field& field)
{
    return !field.field_Constraint.default_val.isEmpty()
    || !field.field_Constraint.Const_Name[TOKEN_DEFAULT].isEmpty();
}

static QString normalizeInsertValue(const DDL::Field& field, const QString& input, bool& quoted)
{
    QString value = unquoteValue(input);

    if (!quoted && value.compare("DEFAULT", Qt::CaseInsensitive) == 0) {
        if (!hasDefaultValue(field)) {
            throw std::invalid_argument(QString("字段 %1 没有默认值")
                                            .arg(field.field_name).toStdString());
        }
        quoted = false;
        return field.field_Constraint.default_val;
    }

    if (nullLike(value) && hasDefaultValue(field)) {
        quoted = false;
        return field.field_Constraint.default_val;
    }

    return nullLike(value) ? value.trimmed() : value;
}

static QString schemaPathFor(const DDL::DataBase& db, const QString& tableName)
{
    return db.path + "/" + tableName + "/" + tableName + ".tbs";
}

QString DML::getTableDataFilePath(const DDL::DataBase& db, const QString& tableName)
{
    return db.path + "/" + tableName + "/" + tableName + ".tbf";
}

QVector<QVector<QString>> DML::loadTableRows(const DDL::DataBase& db, const DDL::Table& table)
{
    QVector<QVector<QString>> rows;
    QFile file(getTableDataFilePath(db, table.name));
    if (!file.open(QIODevice::ReadOnly)) {
        return rows;
    }

    QDataStream in(&file);
    in.setVersion(QDataStream::Qt_5_15);

    int rowCount = 0;
    in >> rowCount;

    int fieldCount = table.fields.size();
    for (int i = 0; i < rowCount; i++) {
        QVector<QString> row;
        for (int j = 0; j < fieldCount; j++) {
            QString value;
            in >> value;
            row.append(value);
        }
        rows.append(row);
    }

    return rows;
}

void DML::saveTableRows(const DDL::DataBase& db, const DDL::Table& table, const QVector<QVector<QString>>& rows)
{
    QString tableDir = db.path + "/" + table.name;
    QDir dir;
    if (!dir.exists(tableDir)) {
        dir.mkpath(tableDir);
    }

    QFile file(getTableDataFilePath(db, table.name));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        throw std::invalid_argument("无法打开表数据文件进行写入");
    }

    QDataStream out(&file);
    out.setVersion(QDataStream::Qt_5_15);

    out << static_cast<int>(rows.size());
    for (const auto& row : rows) {
        for (const auto& value : row) {
            out << value;
        }
    }
}

bool DML::isNullLike(const QString& value)
{
    return nullLike(value);
}

void DML::validateFieldValue(const DDL::Field& field, const QString& value, bool quoted)
{
    if (isNullLike(value)) {
        return;
    }

    QString rawValue = unquoteValue(value);
    bool isQuoted = quoted || isQuotedValue(value);

    switch (field.field_type) {
    case DDL::FieldType::INT: {
        if (isQuoted) {
            throw std::invalid_argument(QString("类型约束违反：字段 %1 应为 INT 类型")
                                            .arg(field.field_name).toStdString());
        }
        bool ok = false;
        rawValue.toInt(&ok);
        if (!ok) {
            throw std::invalid_argument(QString("类型约束违反：字段 %1 应为 INT 类型，实际值为 '%2'")
                                            .arg(field.field_name).arg(rawValue).toStdString());
        }
        break;
    }
    case DDL::FieldType::FLOAT: {
        if (isQuoted) {
            throw std::invalid_argument(QString("类型约束违反：字段 %1 应为 FLOAT 类型")
                                            .arg(field.field_name).toStdString());
        }
        bool ok = false;
        rawValue.toDouble(&ok);
        if (!ok) {
            throw std::invalid_argument(QString("类型约束违反：字段 %1 应为 FLOAT 类型，实际值为 '%2'")
                                            .arg(field.field_name).arg(rawValue).toStdString());
        }
        break;
    }
    case DDL::FieldType::CHAR:
    case DDL::FieldType::VARCHAR:
        if (field.length > 0 && rawValue.length() > field.length) {
            throw std::invalid_argument(QString("长度约束违反：字段 %1 最大长度为 %2")
                                            .arg(field.field_name).arg(field.length).toStdString());
        }
        break;
    default:
        break;
    }
}

QString DML::applyDefault(const DDL::Field& field, const QString& value)
{
    if (!isNullLike(value)) {
        return value;
    }
    if (hasDefaultValue(field)) {
        return field.field_Constraint.default_val;
    }
    return value.trimmed();
}

void DML::validateFieldConstraint(
    const DDL::DataBase& db,
    const DDL::Table& table,
    const DDL::Field& field,
    const QString& value,
    bool quoted,
    int currentRowIndex,
    bool isPrimaryKey)
{
    Q_UNUSED(db);
    Q_UNUSED(table);
    Q_UNUSED(currentRowIndex);
    Q_UNUSED(isPrimaryKey);

    QString effectiveValue = applyDefault(field, value);
    if (field.field_Constraint.not_null && isNullLike(effectiveValue)) {
        throw std::invalid_argument(QString("NOT NULL 约束违反：字段 %1 不能为空")
                                        .arg(field.field_name).toStdString());
    }

    validateFieldValue(field, effectiveValue, quoted && effectiveValue == value);
}

bool DML::hasDuplicateKey(const QVector<QVector<QString>>& rows, int fieldIndex, const QString& value, int excludeRow)
{
    for (int i = 0; i < rows.size(); i++) {
        if (i == excludeRow) {
            continue;
        }
        if (rows[i].size() > fieldIndex && rows[i][fieldIndex] == value) {
            return true;
        }
    }
    return false;
}

bool DML::validateForeignKey(const DDL::DataBase& db, const DDL::Field& field, const QString& value)
{
    if (!field.field_Constraint.Foreign_key || isNullLike(value)) {
        return true;
    }

    const QString refSchemaPath = schemaPathFor(db, field.field_Constraint.ref_table);
    DDL::Table refTable = DDL::loadSchema(refSchemaPath);
    int refFieldIndex = refTable.getFieldIndex(field.field_Constraint.ref_field);
    if (refFieldIndex < 0) {
        return false;
    }

    QVector<QVector<QString>> refRows = loadTableRows(db, refTable);
    for (const auto& row : refRows) {
        if (row.size() > refFieldIndex && row[refFieldIndex] == value) {
            return true;
        }
    }
    return false;
}

int DML::getNextAutoIncrement(const QVector<QVector<QString>>& rows, int fieldIndex)
{
    int maxValue = 0;
    for (const auto& row : rows) {
        if (row.size() <= fieldIndex) {
            continue;
        }
        bool ok = false;
        int value = row[fieldIndex].toInt(&ok);
        if (ok && value > maxValue) {
            maxValue = value;
        }
    }
    return maxValue + 1;
}

static bool tableExists(const DDL::DataBase& db, const QString& tableName)
{
    QFile file(schemaPathFor(db, tableName));
    return file.exists();
}

int DML::executeInsert(const DDL::DataBase& db, const InsertStatement& stmt)
{
    if (db.path.isEmpty()) {
        throw std::invalid_argument("未选择数据库");
    }
    if (!tableExists(db, stmt.tableName)) {
        throw std::invalid_argument(QString("表 %1 不存在").arg(stmt.tableName).toStdString());
    }

    DDL::Table table = DDL::loadSchema(schemaPathFor(db, stmt.tableName));
    QVector<QVector<QString>> rows = loadTableRows(db, table);
    QVector<QVector<QString>> pendingRows;
    QVector<QVector<QString>> rowsForCheck = rows;

    auto validateStoredValue = [&](int fieldIndex, const QString& value, bool quoted) {
        const DDL::Field& field = table.fields[fieldIndex];

        if (field.field_Constraint.not_null && isNullLike(value)) {
            throw std::invalid_argument(QString("NOT NULL 约束违反：字段 %1 不能为空")
                                            .arg(field.field_name).toStdString());
        }

        validateFieldValue(field, value, quoted);

        if (field.field_Constraint.Primary_key) {
            if (isNullLike(value)) {
                throw std::invalid_argument(QString("PRIMARY KEY 约束违反：字段 %1 不能为空")
                                                .arg(field.field_name).toStdString());
            }
            if (hasDuplicateKey(rowsForCheck, fieldIndex, value)) {
                throw std::invalid_argument(QString("PRIMARY KEY 约束违反：字段 %1 的值 '%2' 已存在")
                                                .arg(field.field_name).arg(value).toStdString());
            }
        }

        if (field.field_Constraint.Unique_key) {
            if (!isNullLike(value) && hasDuplicateKey(rowsForCheck, fieldIndex, value)) {
                throw std::invalid_argument(QString("UNIQUE 约束违反：字段 %1 的值 '%2' 已存在")
                                                .arg(field.field_name).arg(value).toStdString());
            }
        }

        if (field.field_Constraint.Foreign_key && !validateForeignKey(db, field, value)) {
            throw std::invalid_argument(QString("FOREIGN KEY 约束违反：字段 %1 的值 '%2' 在被引用表中不存在")
                                            .arg(field.field_name).arg(value).toStdString());
        }
    };

    for (const QVector<QString>& rowValues : stmt.rows) {
        QVector<QString> fullRow(table.fields.size(), "");
        QVector<bool> assigned(table.fields.size(), false);
        QVector<bool> quotedFlags(table.fields.size(), false);

        if (stmt.columns.isEmpty()) {
            if (rowValues.size() != table.fields.size()) {
                throw std::invalid_argument(QString("字段数量不匹配：期望 %1 个，实际 %2 个")
                                                .arg(table.fields.size()).arg(rowValues.size()).toStdString());
            }
            for (int i = 0; i < rowValues.size(); i++) {
                bool quoted = isQuotedValue(rowValues[i]);
                fullRow[i] = normalizeInsertValue(table.fields[i], rowValues[i], quoted);
                quotedFlags[i] = quoted;
                assigned[i] = true;
            }
        } else {
            if (rowValues.size() != stmt.columns.size()) {
                throw std::invalid_argument("字段数量不匹配：列名数量和值数量不一致");
            }
            for (int i = 0; i < stmt.columns.size(); i++) {
                int fieldIndex = table.getFieldIndex(stmt.columns[i]);
                if (fieldIndex < 0) {
                    throw std::invalid_argument(QString("字段 %1 不存在")
                                                    .arg(stmt.columns[i]).toStdString());
                }
                bool quoted = isQuotedValue(rowValues[i]);
                fullRow[fieldIndex] = normalizeInsertValue(table.fields[fieldIndex], rowValues[i], quoted);
                quotedFlags[fieldIndex] = quoted;
                assigned[fieldIndex] = true;
            }
        }

        for (int i = 0; i < table.fields.size(); i++) {
            if (assigned[i]) {
                continue;
            }
            const DDL::Field& field = table.fields[i];
            if (field.field_Constraint.Auto_increasement) {
                fullRow[i] = QString::number(getNextAutoIncrement(rowsForCheck, i));
                quotedFlags[i] = false;
            } else {
                bool quoted = false;
                fullRow[i] = normalizeInsertValue(field, "", quoted);
                quotedFlags[i] = quoted;
            }
        }

        for (int i = 0; i < table.fields.size(); i++) {
            validateStoredValue(i, fullRow[i], quotedFlags[i]);
        }

        pendingRows.append(fullRow);
        rowsForCheck.append(fullRow);
    }

    for (const auto& row : pendingRows) {
        rows.append(row);
    }

    saveTableRows(db, table, rows);
    return pendingRows.size();
}

int DML::executeUpdate(const DDL::DataBase& db, const UpdateStatement& stmt)
{
    if (db.path.isEmpty()) {
        throw std::invalid_argument("未选择数据库");
    }
    if (!tableExists(db, stmt.tableName)) {
        throw std::invalid_argument(QString("表 %1 不存在").arg(stmt.tableName).toStdString());
    }

    DDL::Table table = DDL::loadSchema(schemaPathFor(db, stmt.tableName));
    QVector<QVector<QString>> rows = loadTableRows(db, table);

    int whereIndex = table.getFieldIndex(stmt.whereColumn);
    if (whereIndex < 0) {
        throw std::invalid_argument(QString("WHERE 字段 %1 不存在")
                                        .arg(stmt.whereColumn).toStdString());
    }

    for (auto it = stmt.setMap.begin(); it != stmt.setMap.end(); ++it) {
        if (table.getFieldIndex(it.key()) < 0) {
            throw std::invalid_argument(QString("字段 %1 不存在").arg(it.key()).toStdString());
        }
    }

    int affected = 0;
    for (int rowIndex = 0; rowIndex < rows.size(); rowIndex++) {
        if (rows[rowIndex].size() <= whereIndex || rows[rowIndex][whereIndex] != stmt.whereValue) {
            continue;
        }

        QVector<QString> originalRow = rows[rowIndex];
        for (auto it = stmt.setMap.begin(); it != stmt.setMap.end(); ++it) {
            int fieldIndex = table.getFieldIndex(it.key());
            const DDL::Field& field = table.fields[fieldIndex];
            bool quoted = isQuotedValue(it.value());
            QString value = normalizeInsertValue(field, it.value(), quoted);

            validateFieldValue(field, value, quoted);
            if (field.field_Constraint.not_null && isNullLike(value)) {
                rows[rowIndex] = originalRow;
                throw std::invalid_argument(QString("NOT NULL 约束违反：字段 %1 不能为空")
                                                .arg(field.field_name).toStdString());
            }
            if (field.field_Constraint.Primary_key) {
                if (isNullLike(value) || hasDuplicateKey(rows, fieldIndex, value, rowIndex)) {
                    rows[rowIndex] = originalRow;
                    throw std::invalid_argument(QString("PRIMARY KEY 约束违反：字段 %1 的值 '%2' 不合法")
                                                    .arg(field.field_name).arg(value).toStdString());
                }
            }
            if (field.field_Constraint.Unique_key && !isNullLike(value)
                && hasDuplicateKey(rows, fieldIndex, value, rowIndex)) {
                rows[rowIndex] = originalRow;
                throw std::invalid_argument(QString("UNIQUE 约束违反：字段 %1 的值 '%2' 已存在")
                                                .arg(field.field_name).arg(value).toStdString());
            }
            if (field.field_Constraint.Foreign_key && !validateForeignKey(db, field, value)) {
                rows[rowIndex] = originalRow;
                throw std::invalid_argument(QString("FOREIGN KEY 约束违反：字段 %1 的值 '%2' 在被引用表中不存在")
                                                .arg(field.field_name).arg(value).toStdString());
            }

            rows[rowIndex][fieldIndex] = value;
        }
        affected++;
    }

    if (affected > 0) {
        saveTableRows(db, table, rows);
    }
    return affected;
}

int DML::executeDelete(const DDL::DataBase& db, const DeleteStatement& stmt)
{
    if (db.path.isEmpty()) {
        throw std::invalid_argument("未选择数据库");
    }
    if (!tableExists(db, stmt.tableName)) {
        throw std::invalid_argument(QString("表 %1 不存在").arg(stmt.tableName).toStdString());
    }

    DDL::Table table = DDL::loadSchema(schemaPathFor(db, stmt.tableName));
    QVector<QVector<QString>> rows = loadTableRows(db, table);

    int whereIndex = table.getFieldIndex(stmt.whereColumn);
    if (whereIndex < 0) {
        throw std::invalid_argument(QString("WHERE 字段 %1 不存在")
                                        .arg(stmt.whereColumn).toStdString());
    }

    int before = rows.size();
    for (int i = rows.size() - 1; i >= 0; i--) {
        if (rows[i].size() > whereIndex && rows[i][whereIndex] == stmt.whereValue) {
            rows.removeAt(i);
        }
    }

    int deleted = before - rows.size();
    if (deleted > 0) {
        saveTableRows(db, table, rows);
    }
    return deleted;
}

QString DML::executeSelect(const DDL::DataBase& db, const SelectStatement& stmt)
{
    if (db.path.isEmpty()) {
        throw std::invalid_argument("未选择数据库");
    }
    if (!tableExists(db, stmt.tableName)) {
        throw std::invalid_argument(QString("表 %1 不存在").arg(stmt.tableName).toStdString());
    }

    DDL::Table table = DDL::loadSchema(schemaPathFor(db, stmt.tableName));
    QVector<QVector<QString>> rows = loadTableRows(db, table);

    QString result;
    for (int i = 0; i < table.fields.size(); i++) {
        if (i > 0) result += " | ";
        result += table.fields[i].field_name;
    }
    result += "\n";

    for (int i = 0; i < table.fields.size(); i++) {
        if (i > 0) result += "-+-";
        result += QString().fill('-', table.fields[i].field_name.length());
    }
    result += "\n";

    if (rows.isEmpty()) {
        result += "(empty)";
        return result;
    }

    for (int i = 0; i < rows.size(); i++) {
        for (int j = 0; j < rows[i].size(); j++) {
            if (j > 0) result += " | ";
            result += rows[i][j];
        }
        if (i < rows.size() - 1) {
            result += "\n";
        }
    }
    return result;
}


