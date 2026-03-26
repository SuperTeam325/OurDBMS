#include "DDL.h"
#include <QStringList>
#include <QRegularExpression>
#include <QRegularExpression>

// 辅助函数实现：字符串转FieldType
DDL::FieldType DDL::parseFieldType(const QString& typeStr) {
    QString upperType = typeStr.toUpper();
    if(upperType == "INT") return DDL::FieldType::INT;
    if(upperType == "CHAR") return DDL::FieldType::CHAR;
    if(upperType == "VARCHAR") return DDL::FieldType::VARCHAR;
    if(upperType == "FLOAT") return DDL::FieldType::FLOAT;
    return DDL::FieldType::UNKNOWN;
}
//辅助识别字段名函数
bool isSqlKeyword(const QString& str) {
    QStringList keywords = {"SELECT", "TABLE", "WHERE", "INT", "CHAR", "VARCHAR",
                            "FLOAT", "PRIMARY", "KEY", "NOT", "NULL", "UNIQUE", "DEFAULT"};
    return keywords.contains(str.toUpper());
}

// 提取字段定义中的所有合法字段名
QStringList extractValidFieldNames(const QString& fieldsPart) {
    QStringList validFieldNames;
    // 正则匹配合法字段名：以字母/下划线开头，后跟字母/数字/下划线（完整匹配字段名）
    // 匹配规则：字段名 + 空格 + 类型（如 id INT / stu_name VARCHAR(20)）
    QRegularExpression fieldNameRegex(R"(\b([a-zA-Z_][a-zA-Z0-9_]*)\b\s+[a-zA-Z]+)");
    QRegularExpressionMatchIterator it = fieldNameRegex.globalMatch(fieldsPart);

    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        QString fieldName = match.captured(1);
        // 排除SQL保留字
        if (!isSqlKeyword(fieldName)) {
            validFieldNames.append(fieldName);
        }
    }
    return validFieldNames;
}

// 解析CREATE TABLE语句
DDL::Table DDL::parseCreateTable(const QString& sql) {
    // 清洗SQL语句
    QString cleanedSql = sql;
    cleanedSql = cleanedSql.remove(QRegularExpression("--[^\n]*"));
    cleanedSql = cleanedSql.remove('\n').remove('\r');
    cleanedSql = cleanedSql.simplified();

    if(cleanedSql.trimmed().isEmpty()) {
        throw std::invalid_argument("错误！空语句");
    }

    qDebug()<<"清洗后的SQL语句:"<<cleanedSql;
    //不允许中文标点
    QString chinesePuncts = "，。！？；：""''（）【】[]、·￥……—～《》〈〉「」『』｛｝｜､；：！？｡＂＇｀｢｣、，。·、；：！？…—ˉ‖’’’”“〝〞∕｜〒〔〕〈〉《》「」『』．〖〗【】（）［］｛｝";
    bool hasChinesePunct = false;
    for (const QChar& ch : chinesePuncts) {
        if (cleanedSql.contains(ch)) {
            hasChinesePunct = true;
            break;
        }
    }
    if (hasChinesePunct) {
        throw std::invalid_argument("SQL格式错误：禁止使用中文标点（如，。；：（）、等），请替换为英文标点");
    }

    //找表名范围 mid():从字符串的指定位置开始，截取指定长度的子字符串.但不会改变字符串本身
    QString createTableStr = "CREATE TABLE";
    int createTableEnd = createTableStr.length();

    //提取表名（规则：CREATE TABLE后，直到第一个非字母/数字/下划线的字符为止）
    QString tableNamePart = cleanedSql.mid(createTableEnd).trimmed();
    QRegularExpression tableNameRegex("^([a-zA-Z0-9_]+)"); // 表名只能是字母/数字/下划线
    QRegularExpressionMatch match = tableNameRegex.match(tableNamePart);
    if (!match.hasMatch()) { // 检查是否匹配成功
        throw std::invalid_argument("SQL格式错误：表名格式非法（仅允许字母、数字、下划线）");
    }
    QString tableName = match.captured(1); // 提取第一个分组（表名）
    int tableNameLength = tableName.length();
    if(tableName.isEmpty()) {
        throw std::invalid_argument("表名不能为空");
    }
    qDebug()<<cleanedSql;
    //检验第一个开括号
    int tableNameEndPos = createTableEnd + tableNamePart.indexOf(tableName) + tableNameLength;
    QString bracketStart_part = cleanedSql.mid(tableNameEndPos+1,2).trimmed();
    qDebug()<<"左括号部分"<<bracketStart_part; //只会有这一个开括号，没有就是没有
    int bracketStart = tableNameEndPos+3;
    if(bracketStart_part.indexOf("(") == -1) {
        throw std::invalid_argument("SQL格式错误：缺少字段定义的左括号");
    }
    //检查闭合括号
    QString bracketEnd_part=cleanedSql.right(2);
    int bracketEnd =cleanedSql.length()-1;
    if(bracketEnd_part.lastIndexOf(')') == -1) {
        throw std::invalid_argument("SQL格式错误：缺少闭合括号");
    }
    QString trimmedSql = cleanedSql.trimmed();

    // 检查最后一个字符是否是;
    if (trimmedSql.back() != ';') {
        throw std::invalid_argument("SQL格式错误：语句必须以分号 ; 结尾");
    }
    // 检查分号在闭合括号之后
    int semicolonPos = trimmedSql.lastIndexOf(';');
    if (semicolonPos < bracketEnd) {
        throw std::invalid_argument("SQL格式错误：分号不能出现在闭合括号之前");
    }

    //提取字段定义部分
    QString fieldsPart = cleanedSql.mid(bracketStart+1, bracketEnd - bracketStart -2).trimmed();
    if(fieldsPart.isEmpty()) {
        throw std::invalid_argument("字段定义不能为空");
    }
    qDebug()<<"字段集合"<<fieldsPart;

    //字段部分不允许出现分号
    if(fieldsPart.contains(";")){
        throw std::invalid_argument("分号错误");
    }

    //检查最后一个字段后是否有多余逗号
    if (fieldsPart.trimmed().endsWith(',')) {
        throw std::invalid_argument("字段定义错误：最后一个字段后存在多余逗号");
    }

    //检测缺少逗号
    //1.没逗号的情况
    if(!fieldsPart.contains(",")) throw std::invalid_argument("错误，缺少逗号");

    QStringList validFieldDefs = fieldsPart.split(',', Qt::SkipEmptyParts);

    //2.是否缺少逗号
    QStringList validFieldNames = extractValidFieldNames(fieldsPart);
    if (validFieldNames.isEmpty()) {
        throw std::invalid_argument("字段定义错误：未识别到合法的字段名");
    }
    qDebug()<<"识别到的合法字段名："<<validFieldNames;

    // 计算理论正确逗号数（合法字段数 - 1）
    int expectedCommaCount = validFieldNames.size() - 1;

    //统计实际英文逗号数
    int actualCommaCount = fieldsPart.count(',');

    // 校验逗号数量
    if (actualCommaCount < expectedCommaCount) {
        throw std::invalid_argument(QString("字段定义错误")
                                        .arg(expectedCommaCount - actualCommaCount)
                                        .arg(validFieldNames.size())
                                        .arg(expectedCommaCount)
                                        .arg(actualCommaCount).toStdString());
    } else if (actualCommaCount > expectedCommaCount) {
        throw std::invalid_argument(QString("字段定义错误")
                                        .arg(actualCommaCount - expectedCommaCount)
                                        .arg(validFieldNames.size())
                                        .arg(expectedCommaCount)
                                        .arg(actualCommaCount).toStdString());
    }


    //提取字段定义，按照,提取各个部分存储在QStringList中
    // 保留空字符串，用于检测多余逗号
    QStringList fieldDefs = fieldsPart.split(',', Qt::KeepEmptyParts);
    DDL::Table table;
    table.name = tableName;

    for(int i = 0; i < fieldDefs.size(); i++) { // 改为索引遍历，方便定位错误位置
        QString def = fieldDefs[i];
        QString fieldDef = def.trimmed();

        // 检测空字段（多余逗号，目前局限于连着的两个逗号）
        if(fieldDef.isEmpty()) {
            throw std::invalid_argument(QString("字段定义错误：存在多余逗号（第%1个字段位置为空）").arg(i+1).toStdString());
        }

        //按空格拆分最小语义单元并跳过拆分产生的空字符串
        QStringList parts = fieldDef.split(' ', Qt::SkipEmptyParts);

        //至少要有 字段名+类型
        if(parts.size() < 2) {
            throw std::invalid_argument(QString("字段定义错误：%1").arg(fieldDef).toStdString());
        }

        QString fieldName = parts[0];
        QStringList sqlKeywords = {"SELECT", "TABLE", "WHERE", "INT", "CHAR", "VARCHAR", "FLOAT", "PRIMARY", "KEY", "NOT", "NULL"};
        if (sqlKeywords.contains(fieldName.toUpper())) {
            throw std::invalid_argument(QString("字段名错误：%1 是SQL保留字，禁止直接使用").arg(fieldName).toStdString());
        }
        QString typePart = parts[1];
        DDL::FieldType type = DDL::parseFieldType(typePart);
        uint16_t length = 0;
        //处理带长度的类型
        int lenBracketStart = typePart.indexOf('(');
        if(lenBracketStart != -1) {
            int lenBracketEnd = typePart.indexOf(')');
            if(lenBracketEnd != -1) {
                QString lenStr = typePart.mid(lenBracketStart+1, lenBracketEnd - lenBracketStart -1);
                //转为无符号短整型作为长度
                length = lenStr.toUShort();
                // 去掉括号，只保留纯类型字符串（比如 VARCHAR(20) → "VARCHAR"）
                typePart = typePart.left(lenBracketStart);//保留“(”的左边
                type = DDL::parseFieldType(typePart);
            }
        }
        //处理约束
        DDL::FieldConstraint constraint;
        for(int i=2; i<parts.size(); i++) {
            QString part = parts[i].toUpper();
            if(part == "NOT") {
                if(i+1 < parts.size() && parts[i+1].toUpper() == "NULL") {
                    constraint.not_null = true;
                    i++;
                }else{
                    throw std::invalid_argument("字段定义错误！");
                }
            } else if(part == "PRIMARY") {
                if(i+1 < parts.size() && parts[i+1].toUpper() == "KEY") {
                    constraint.Primary_key = true;
                    i++;
                }
            } else if(part == "UNIQUE") {
                constraint.Unique_key = true;
            } else if(part == "DEFAULT") {
                if(i+1 < parts.size()) {
                    constraint.default_val = parts[i+1];
                    i++;
                }
            }
        }

        DDL::Field field(fieldName, type, length, constraint);
        table.fields.append(field);
    }

    return table;
}
