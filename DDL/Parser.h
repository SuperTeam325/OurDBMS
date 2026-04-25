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
    void paraseDTableF(const QString &sql,const QString&,DDL::DataBase&);
    void paraseDTKEY(const QString &sql,DDL::DataBase&);
    void paraseAddCol(const QString &sql,DDL::DataBase&);
    void paraseAddCS(const QString &sql,DDL::DataBase& db);//添加约束
    void paraseModifyCol(const QString &sql,DDL::DataBase&);
    DDL::FieldType parseFieldType(const QString&);
private:
    Lexer le;
    DDL::Table table;

    QList<Token> tokens;
    int pos;         // 当前token的索引
    Token peek();    // 看当前 Token
    int getPos(TokenType type);
    QString getText(int);
    void next();     // 下一个
    void match(TokenType type); // 匹配必须出现的Token
    QString hasForeign(DDL::Table&,DDL::DataBase&);
};

#endif // PARSER_H
