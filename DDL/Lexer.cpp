#include "lexer.h"

QList<Token> Lexer::ReadSQL(const QString& sql)
{
    QList<Token> tokens;
    int i = 0;
    int len = sql.length();

    while (i < len) {
        QChar c = sql[i];

        // 1. 跳过空格、换行、制表符
        if (c.isSpace()) {
            i++;
            continue;
        }

        // 2. 符号：( ) , ; 单独成Token
        if (c == '(') {
            tokens.append(Token(TOKEN_LPAREN, "("));
            i++;
            continue;
        }
        if (c == ')') {
            tokens.append(Token(TOKEN_RPAREN, ")"));
            i++;
            continue;
        }
        if (c == ',') {
            tokens.append(Token(TOKEN_COMMA, ","));
            i++;
            continue;
        }
        if (c == ';') {
            tokens.append(Token(TOKEN_SEMICOLON, ";"));
            i++;
            continue;
        }
        // 3. 字母/下划线 → 读完整单词（关键字、标识符）

        if (c.isLetter() || c == '_') {
            int start = i;
            while (i < len && (sql[i].isLetterOrNumber() || sql[i] == '_')) {
                i++;
            }
            QString word = sql.mid(start, i - start);
            TokenType type = checkKeyword(word);
            tokens.append(Token(type, word));
            continue;
        }

        // 4. 数字
        if (c.isDigit()) {
            int start = i;
            while (i < len && sql[i].isDigit()) {
                i++;
            }
            QString num = sql.mid(start, i - start);
            tokens.append(Token(TOKEN_NUMBER, num));
            continue;
        }

        i++;
    }

    tokens.append(Token(TOKEN_EOF, ""));
    return tokens;
}

// 关键字判断
TokenType Lexer::checkKeyword(const QString& word)
{
    QString upper = word.toUpper();

    if (upper == "CREATE") return TOKEN_CREATE;
    if(upper == "USE") return TOKEN_USE;
    if (upper == "TABLE") return TOKEN_TABLE;
    if(upper == "DATABASE") return TOKEN_DATABASE;
    if(upper == "ALTER")  return TOKEN_ALTER;
    if(upper == "DROP") return TOKEN_DROP;
    if(upper == "ADD")  return TOKEN_ADD;
    if(upper == "MODIFY")  return TOKEN_MODIFY;
    if(upper == "COLUMN") return TOKEN_COLUMN;
    if (upper == "INT") return TOKEN_INT;
    if (upper == "FLOAT") return TOKEN_FLOAT;
    if (upper == "CHAR") return TOKEN_CHAR;
    if (upper == "VARCHAR") return TOKEN_VARCHAR;
    if (upper == "NOT") return TOKEN_NOT;
    if (upper == "NULL") return TOKEN_NULL;
    if (upper == "PRIMARY") return TOKEN_PRIMARY;
    if (upper == "KEY") return TOKEN_KEY;
    if (upper == "UNIQUE") return TOKEN_UNIQUE;
    if (upper == "DEFAULT") return TOKEN_DEFAULT;
    if (upper == "AUTO_INCREMENT") return TOKEN_AUTO_INCREMENT;
    if(upper=="FOREIGN") return TOKEN_FOREIGN;
    if(upper=="REFERENCES") return TOKEN_REFERENCES;

    if (upper == "CONSTRAINT") return TOKEN_CONSTRAINT;

    return TOKEN_IDENTIFIER;
}

bool Lexer::isNumber(const QString& word)
{
    bool ok;
    word.toInt(&ok);
    return ok;
}
