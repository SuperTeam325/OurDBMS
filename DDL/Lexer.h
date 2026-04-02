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
    TOKEN_CONSTRAINT,

    TOKEN_NUMBER,
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
