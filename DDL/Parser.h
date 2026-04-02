#ifndef PARSER_H
#define PARSER_H
#include "Lexer.h"
#include "DDL.h"
#include <QList>

class Parser {
public:
    Parser(QList<Token> tokens);
    Parser();
    DDL::Table parseCreateTable(const QString&,DDL::DataBase&);  // 解析 CREATE TABLE，返回表结构
    DDL:: DataBase paraseCreateDB(const QString&, QString path); //解析 CREATE DATABASE
    void paraseUSEDB(const QString&,DDL::DataBase&);
    DDL::FieldType parseFieldType(const QString&);
private:
    Lexer le;
    DDL::Table table;

    QList<Token> tokens;
    int pos;         // 当前读到第几个Token
    Token peek();    // 看当前Token
    int getPos(TokenType type);
    QString getText(int);
    void next();     // 下一个
    void match(TokenType type); // 匹配必须出现的Token
};

#endif // PARSER_H
