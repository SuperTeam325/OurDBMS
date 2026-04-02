#include "Parser.h"
#include <QException>
#include <stdexcept>
#include<QDebug>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDir>
Parser::Parser(QList<Token> tokens) {
    this->tokens = tokens;
    pos = 0;
}

Parser::Parser(){
    pos=0;
}


Token Parser::peek(){
    if(pos<tokens.size())
        return tokens[pos];
    return Token(TOKEN_EOF);

}

void Parser::next(){
    pos++;
}

int Parser::getPos(TokenType type){
    int i=0;
    for(auto t:tokens){
        i++;
        if(t.type==type) return i;
    }
    return -1;
}

QString Parser::getText(int pos){
    return tokens[pos].text;

}

void Parser::match(TokenType type){
    if (peek().type == type) {
        next();
    } else {
        throw std::invalid_argument(QString("语法错误,near %1").arg(getText(pos-1)).toStdString());
    }
}

//后续有了多用户功能后我将为每个用户创建单独的文件夹，里面再存这些schema.dbs，
//student.dbf，db_config.json这些文件来区分，避免混乱

// 辅组函数，从student.dbf，db_config.json读取数据库文件地址
QString getDbPathByName(const QString& dbName)
{
    //  打开配置文件
    QFile file("db_config.json");

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return "";
    }

    // 读取全部内容
    QByteArray data = file.readAll();
    file.close();

    // 转成 JSON
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || !doc.isObject()) {
        return "";
    }

    QJsonObject obj = doc.object();

    // 根据数据库名拿路径
    if (obj.contains(dbName)) {
        return obj[dbName].toString();
    }

    return "";
}

//从数据库的表结构文件读取表名是否存在
bool isTableExists(QString path, QString Tname){

    QFile file(path);
    if(!file.open(QIODevice::ReadOnly)) qDebug()<<path+"打开失败";

    QDataStream in(&file);

    QString name;
     while (!in.atEnd()){
        in>>name;
        if(name==Tname) return true;
    }


    return false;
}

DDL::FieldType Parser::parseFieldType(const QString& typeStr){


      QString s = typeStr.toUpper();
    if(s=="INT")   return DDL::FieldType::INT;
    if (s == "VARCHAR")  return DDL::FieldType::VARCHAR;
    if (s == "CHAR")     return DDL::FieldType::CHAR;
    if (s == "FLOAT")    return DDL::FieldType::FLOAT;

    return DDL::FieldType::UNKNOWN;
}

DDL::Table Parser::parseCreateTable(const QString& sql,DDL::DataBase& db){
    //重置
    tokens.clear();
    pos=0;
    //获取被差分成多个Toekn的sql
    tokens=le.ReadSQL(sql);

    QString field_name;
    DDL::FieldType field_type;
    uint16_t length=0;
    DDL::FieldConstraint field_Constraint;

    match(TOKEN_CREATE);
    match(TOKEN_TABLE);

    if(peek().type==TOKEN_IDENTIFIER){
        table.name=peek().text;

        next();
    }else{
       throw std::invalid_argument("语法错误：缺少表名");
    }
    //检验这个数据库是否存在同名表
    QString dbPath =db.path+"/"+db.name+".dbs";
    bool exists = isTableExists(dbPath, table.name);
    if (exists) {
       throw std::invalid_argument("失败，表已存在！");
    } else {
        qDebug() << "表不存在，可以创建！";
    }

    match(TOKEN_LPAREN);
    //循环解析字段
    while(peek().type!=TOKEN_EOF){


        if(peek().type==TOKEN_IDENTIFIER){
            field_name=peek().text;
            next();
        }else{
            throw std::invalid_argument(QString("语法错误: near %1").arg(peek().text).toStdString());
        }


        field_type=parseFieldType(peek().text);
        if(field_type==DDL::FieldType::UNKNOWN){
            throw std::invalid_argument(QString("暂时不支持该字段类型%1").arg(peek().text).toStdString());
        }
        if(peek().type==TOKEN_CHAR || peek().type==TOKEN_VARCHAR){
            next();
            match(TOKEN_LPAREN);
            length = (uint16_t)peek().text.toUShort();
            match(TOKEN_NUMBER);
            match(TOKEN_RPAREN);
        }else{
            next();
        }


        //解析字段约束
        while(true){

            Token t=peek();

            if(t.type==TOKEN_PRIMARY){
                next();
                match(TOKEN_KEY);
                field_Constraint.Primary_key=true;
            }else if(t.type==TOKEN_NOT){
                next();
                match(TOKEN_NULL);
                field_Constraint.not_null=true;
            }else if(t.type==TOKEN_UNIQUE){
                next();
                field_Constraint.Unique_key=true;
            }else if(t.type==TOKEN_DEFAULT){
                next();
                field_Constraint.default_val=peek().text;
                next();
            }
            else{
                break;
            }

        }
        DDL::Field field(field_name,field_type,length,field_Constraint);
        table.fields.append(field);

        if(peek().type==TOKEN_COMMA){
            next();
            continue;
        }else if(peek().type==TOKEN_RPAREN){
            next();
            match(TOKEN_SEMICOLON);
            continue;
        }else{
            match(TOKEN_COMMA);
        }

    }

    return table;
}

DDL::DataBase Parser::paraseCreateDB(const QString& sql,QString path){
    QString Mainpath="C:/Users/21495/Desktop/DBMS测试";
    //重置
    tokens.clear();
    pos=0;
    tokens=le.ReadSQL(sql);
    //存储数据库信息的配置文件
    QFile file("db_config.json");
    QJsonObject obj;

    DDL::DataBase db;
    QString name;

    match(TOKEN_CREATE);
    match(TOKEN_DATABASE);
    if(peek().type==TOKEN_IDENTIFIER){
        name=peek().text;
        next();
    }else{
        throw::std::invalid_argument(QString("语法错误，near %1").arg(peek().type).toStdString());
    }
    match(TOKEN_SEMICOLON);

    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QByteArray data = file.readAll();
        file.close();

        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (!doc.isNull()) {
            obj = doc.object();
        }
    }
    if(obj.contains(name)){
        throw::std::invalid_argument("已存在同名数据库");
    }
    if(!path.isEmpty()){
        obj[name]=path+"/"+name;
    }else{
        obj[name]=Mainpath+"/"+name;
    }


    db.name=name;
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QJsonDocument doc(obj);
        file.write(doc.toJson());
        file.close();
    }
    QString finalPath = obj[name].toString();
    //建数据库文件夹
    QDir dir;
    if (!dir.exists(finalPath)) {
        dir.mkpath(finalPath);
    }

    //同时生成一个对于的同名二进制文件来存储数据库的表结构信息
    QString dbsPath=finalPath+"/"+db.name+".dbs";
    QFile f(dbsPath);

    // 如果文件已经存在，先删除
    if (f.exists()) {
        f.remove();
    }

    // 以【只写 + 二进制】模式打开 → 自动创建空文件
    if (f.open(QIODevice::WriteOnly)) {
        // 什么都不用写！打开再关闭就是空文件
        f.close();
    }

    return db;
}
//操作USE DB
void Parser::paraseUSEDB(const QString& sql,DDL::DataBase& db){
    QString path;

    //重置
    tokens.clear();
    pos=0;
    tokens=le.ReadSQL(sql);

    match(TOKEN_USE);
    if(peek().type==TOKEN_IDENTIFIER){
        path= getDbPathByName(peek().text);
        if(path.isEmpty()){
            throw::std::invalid_argument("该数据库不存在");
        }
        db.name=peek().text;
        next();
    }else{
        throw::std::invalid_argument(QString("语法错误，near %1").arg(peek().type).toStdString());
    }
    match(TOKEN_SEMICOLON);


    db.path=path;
}


