#ifndef LEXER_H
#define LEXER_H

#include <QString>
#include <QList>

// TokenType
enum TokenType {
    TOKEN_CREATE,
    TOKEN_USE,
    TOKEN_DROP,
    TOKEN_ALTER,
    TOKEN_DATABASE, //数据库名
    TOKEN_TABLE,
    TOKEN_ADD,
    TOKEN_MODIFY,
    TOKEN_COLUMN,
    //类型
    TOKEN_INT,
    TOKEN_FLOAT,
    TOKEN_CHAR,
    TOKEN_VARCHAR,
    //名字
    TOKEN_IDENTIFIER,
    //标点
    TOKEN_LPAREN,    // (
    TOKEN_RPAREN,    // )
    TOKEN_COMMA,     // ,
    TOKEN_SEMICOLON, // ;
    //关键字约束
    TOKEN_NOT,
    TOKEN_NULL,
    TOKEN_PRIMARY,
    TOKEN_KEY,
    TOKEN_UNIQUE,
    TOKEN_DEFAULT,
    TOKEN_AUTO_INCREMENT,
    TOKEN_FOREIGN,
    TOKEN_REFERENCES,
    TOKEN_CONSTRAINT,//表级约束

    // DML 关键字
    TOKEN_INSERT,
    TOKEN_INTO,
    TOKEN_VALUES,
    TOKEN_UPDATE,
    TOKEN_SET,
    TOKEN_WHERE,
    TOKEN_DELETE,
    TOKEN_FROM,
    TOKEN_AND,
    TOKEN_SELECT,
    TOKEN_STAR,   // 通配符 *

    TOKEN_NUMBER,
    TOKEN_STRING,    // 单引号字符串
    TOKEN_EQUAL,     // =
    TOKEN_EOF
};

struct Token {
    TokenType type;
    QString text;
    Token(TokenType t = TOKEN_EOF, QString tx = "") : type(t), text(tx) {}
};

class Lexer {
public:
    QList<Token> ReadSQL(const QString& sql);  // 分词主函数

private:
    TokenType checkKeyword(const QString& word); // 判断是不是关键字
    bool isNumber(const QString& word);          // 判断是不是数字
};

#endif // LEXER_H
