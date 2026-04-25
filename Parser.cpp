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

//添加外键
QString Parser::hasForeign(DDL::Table& table,DDL::DataBase& db){
        qDebug()<<"到这";
        next();
        match(TOKEN_KEY);
        match(TOKEN_LPAREN);
        int index=0;
        DDL::FieldType ft;
        if(peek().type==TOKEN_IDENTIFIER){
            if(table.hasField(peek().text)){
                index=table.getFieldIndex(peek().text);
                ft=table.fields[index].field_type;
            }else{
                throw std::invalid_argument(QString("不存此字段: near %1").arg(peek().text).toStdString());
            }
        }else{
            qDebug()<<"到这";
            throw std::invalid_argument(QString("语法错误: near %1").arg(peek().text).toStdString());

        }
        next();
        match(TOKEN_RPAREN);

        match(TOKEN_REFERENCES);
        QString path=db.path+"/"+db.name+".dbs";
        QStringList TNlist{};

        if(peek().type==TOKEN_IDENTIFIER){
            TNlist=DDL::readFromDbs(path);
            if(!TNlist.contains(peek().text)){
                throw std::invalid_argument(QString("无此表: near %1").arg(peek().text).toStdString());
            }
        }else{
            throw std::invalid_argument(QString("无表名: near %1").arg(peek().text).toStdString());
        }

        QString Tname=peek().text;//拿到表名
        QString Tpath=db.path+"/"+Tname+"/"+Tname+".tbs";

        next();
        match(TOKEN_LPAREN);
        DDL::Table t{};
        QString fN;
        if(peek().type==TOKEN_IDENTIFIER){
            fN=peek().text;
            t=DDL::loadSchema(Tpath);
            qDebug()<<"关联字段名"<<fN;
            if(t.hasField(fN)){
                int i=t.getFieldIndex(fN);
                if(!t.fields[i].field_Constraint.Primary_key){
                    throw std::invalid_argument(QString("语法错误,关联字段非主键: near %1").arg(peek().text).toStdString());

                }else{
                    if(t.fields[i].field_type!=ft){
                        throw std::invalid_argument(QString("语法错误,关联字段类型不匹配: near %1").arg(peek().text).toStdString());

                    }
                }
            }else{
                throw std::invalid_argument(QString("语法错误,关联字段不存在: near %1").arg(peek().text).toStdString());

            }
        }

        //确认可以是外键
        table.fields[index].field_Constraint.Foreign_key=true;
        table.fields[index].field_Constraint.ref_table=Tname;  // 保存被引用表名
        table.fields[index].field_Constraint.ref_field=fN;     // 保存被引用字段名
        next();
        match(TOKEN_RPAREN);
        return fN;
}

DDL::Table Parser::parseCreateTable(const QString& sql,DDL::DataBase& db){
    //重置
    tokens.clear();
    pos=0;
    table.fields.clear();
    //获取被差分成多个Toekn的sql
    tokens=le.ReadSQL(sql);

     //主键数量限制
    bool isOnePK=false;

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
        QString field_name;
        DDL::FieldType field_type;
        uint16_t length=0;
        int Ccount=0;
        DDL::FieldConstraint field_Constraint;

        //解析表级约束Constraint
        if(peek().type==TOKEN_CONSTRAINT){
            QString Cname;
            next();
            if(peek().type==TOKEN_IDENTIFIER){
                Cname=peek().text;
                next();
            }
            Token t=peek();

            if(t.type==TOKEN_PRIMARY){
                if(!isOnePK){
                    next();
                    match(TOKEN_KEY);
                    field_Constraint.Primary_key=true;
                    isOnePK=true;
                }else{
                    throw std::invalid_argument("主键必须唯一");
                }
            }else if(t.type==TOKEN_NOT){
                next();
                match(TOKEN_NULL);
                throw std::invalid_argument(QString("语法错误不支持NOT NULL: near %1").arg(peek().text).toStdString());

            }else if(t.type==TOKEN_UNIQUE){
                next();
                field_Constraint.Unique_key=true;

                //qDebug()<<"email约束:"<<field_Constraint.Const_Name[t.type];

            }else if(t.type==TOKEN_DEFAULT){
                next();
                field_Constraint.default_val=peek().text;

                next();
            }else if(t.type==TOKEN_AUTO_INCREMENT){
                next();
                field_Constraint.Auto_increasement=true;

            }else if(t.type==TOKEN_FOREIGN){
                QString fN = hasForeign(table,db);
                int i = table.getFieldIndex(fN);
                table.fields[i].field_Constraint.Const_Name[TOKEN_FOREIGN]=Cname;

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
            }else{
                throw std::invalid_argument(QString("语法错误,无约束: near %1").arg(peek().text).toStdString());

            }
            match(TOKEN_LPAREN);
            bool isHave=false;
            if(peek().type==TOKEN_IDENTIFIER){
                for(int i=0;i<table.fields.size();++i){
                    if(table.fields[i].field_name==peek().text){
                        qDebug()<<table.fields[i].field_name;
                        // 合并约束，保留原来的，只新增表级约束
                        if (field_Constraint.Primary_key)
                            table.fields[i].field_Constraint.Primary_key = true;
                        if (field_Constraint.Unique_key)
                            qDebug()<<"更新"<<table.fields[i].field_name<<"UNIQUE约束";
                            table.fields[i].field_Constraint.Unique_key = true;
                        if (!field_Constraint.default_val.isEmpty())
                            table.fields[i].field_Constraint.default_val = field_Constraint.default_val;
                        if (field_Constraint.Auto_increasement)
                            table.fields[i].field_Constraint.Auto_increasement = true;
                        if(field_Constraint.Foreign_key)
                            table.fields[i].field_Constraint.Foreign_key = true;


                        table.fields[i].field_Constraint.Const_Name[t.type]=Cname;
                        isHave=true;
                        break;
                    }
                }
                if(!isHave){

                    throw std::invalid_argument(QString("语法错误: near %1").arg(peek().text).toStdString());
                }
            }else{
                throw std::invalid_argument(QString("语法错误: near %1").arg(peek().text).toStdString());
            }
            next();
            match(TOKEN_RPAREN);

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
        //解析外键
           if(peek().type==TOKEN_FOREIGN){
               hasForeign(table,db);
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


        //字段分析
        if(peek().type==TOKEN_IDENTIFIER){
            bool isHave=false;
            for(int i=0;i<table.fields.size();++i){
                if(table.fields[i].field_name==peek().text){
                    qDebug()<<table.fields[i].field_name;
                    isHave=true;
                    break;
                }
            }
            if(isHave){
               throw std::invalid_argument(QString("语法错误(字段名重复): near %1").arg(peek().text).toStdString());
            }
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
                if(!isOnePK){
                    next();
                    match(TOKEN_KEY);
                    field_Constraint.Primary_key=true;
                    field_Constraint.not_null=true;
                    isOnePK=true;
                }else{
                    throw std::invalid_argument("主键必须唯一");
                }

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
            }else if(t.type==TOKEN_AUTO_INCREMENT){
                next();
                field_Constraint.Auto_increasement=true;
            }else if(t.type==TOKEN_FOREIGN){
                // 行内 FOREIGN KEY 语法：FOREIGN KEY REFERENCES ref_table(ref_field)
                next(); // consume FOREIGN
                match(TOKEN_KEY);
                match(TOKEN_REFERENCES);

                // 获取被引用表名
                if(peek().type!=TOKEN_IDENTIFIER){
                    throw std::invalid_argument(QString("语法错误: 期望表名 near %1").arg(peek().text).toStdString());
                }
                QString refTableName = peek().text;
                next();

                // 检查被引用表是否存在
                QString dbDbsPath = db.path+"/"+db.name+".dbs";
                QStringList tableList = DDL::readFromDbs(dbDbsPath);
                if(!tableList.contains(refTableName)){
                    throw std::invalid_argument(QString("语法错误: 不存在被引用的表 %1").arg(refTableName).toStdString());
                }

                // 读取被引用字段
                match(TOKEN_LPAREN);
                if(peek().type!=TOKEN_IDENTIFIER){
                    throw std::invalid_argument(QString("语法错误: 期望字段名 near %1").arg(peek().text).toStdString());
                }
                QString refFieldName = peek().text;
                next();
                match(TOKEN_RPAREN);

                // 验证被引用表的主键字段存在
                QString refSchemaPath = db.path + "/" + refTableName + "/" + refTableName + ".tbs";
                DDL::Table refTable = DDL::loadSchema(refSchemaPath);
                if(!refTable.hasField(refFieldName)){
                    throw std::invalid_argument(QString("语法错误: 被引用表 %1 中不存在字段 %2").arg(refTableName).arg(refFieldName).toStdString());
                }
                int refFieldIdx = refTable.getFieldIndex(refFieldName);
                if(!refTable.fields[refFieldIdx].field_Constraint.Primary_key){
                    throw std::invalid_argument(QString("语法错误: 被引用字段 %1.%2 必须为主键").arg(refTableName).arg(refFieldName).toStdString());
                }
                // 验证字段类型匹配
                if(refTable.fields[refFieldIdx].field_type != field_type){
                    throw std::invalid_argument(QString("语法错误: 外键字段类型与被引用字段类型不匹配").toStdString());
                }

                // 设置外键约束
                field_Constraint.Foreign_key = true;
                field_Constraint.ref_table = refTableName;
                field_Constraint.ref_field = refFieldName;
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
    //默认路径
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

//ALTER TABLE DROP Field
void Parser::paraseDTableF(const QString &sql,const QString& path,DDL::DataBase& db){
    if(path.isEmpty()){
         throw::std::invalid_argument("未指定数据库");
    }
    //重置
    tokens.clear();
    pos=0;
    tokens=le.ReadSQL(sql);

    DDL::Table table{};

    QString Tname;
    QString dbPath;
    match(TOKEN_ALTER);
    match(TOKEN_TABLE);

    if(peek().type==TOKEN_IDENTIFIER){
        Tname=peek().text;
        //检验这个数据库是否存在同名表
       dbPath =getDbPathByName(db.name);
        bool exists = isTableExists(dbPath+"/"+db.name+".dbs", Tname);
       qDebug()<<"表名"<<Tname;
        if (!exists) {
            throw std::invalid_argument("失败，表不存在！");
        }
        next();
    }else{
         throw::std::invalid_argument(QString("语法错误，near %1").arg(peek().type).toStdString());
    }
    table=DDL::loadSchema(dbPath+"/"+Tname+"/"+Tname+".tbs");

    while(peek().type!=TOKEN_EOF){

        match(TOKEN_DROP);
        match(TOKEN_COLUMN);


        if(peek().type==TOKEN_IDENTIFIER){

            if(!table.hasField(peek().text)){
                throw std::invalid_argument("失败，字段不存在！");
            }else{
                table.fields.remove(table.getFieldIndex(peek().text));
                next();
            }
        }else{

            throw::std::invalid_argument(QString("无字段名，near %1").arg(peek().type).toStdString());
        }

        if(peek().type==TOKEN_COMMA){
            next();
            continue;
        }else{
            match(TOKEN_SEMICOLON);
            continue;
        }
    }

    DDL::saveSchema(table,db.path);
}
//删除约束 DROP 约束 约束名
void Parser:: paraseDTKEY(const QString &sql,DDL::DataBase& db){
    if(db.path.isEmpty()){
        throw::std::invalid_argument("未指定数据库");
    }
    //重置
    tokens.clear();
    pos=0;
    tokens=le.ReadSQL(sql);

    DDL::Table table{};

    QString Tname;
    QString dbPath;

    match(TOKEN_ALTER);
    match(TOKEN_TABLE);
    if(peek().type==TOKEN_IDENTIFIER){
        Tname=peek().text;
        //检验这个数据库是否存在同名表
        dbPath =getDbPathByName(db.name);
        bool exists = isTableExists(dbPath+"/"+db.name+".dbs", Tname);
        if (!exists) {
            throw std::invalid_argument("失败，表不存在！");
        }
        next();
    }else{

        throw::std::invalid_argument(QString("语法错误，near %1").arg(peek().type).toStdString());
    }

    table=DDL::loadSchema(dbPath+"/"+Tname+"/"+Tname+".tbs");

    while(peek().type!=TOKEN_EOF){

        match(TOKEN_DROP);

        Token t=peek();

        if(t.type==TOKEN_PRIMARY){
            next();
            match(TOKEN_KEY);
            for(auto& f:table.fields){
                if(f.field_Constraint.Primary_key){
                    f.field_Constraint.Primary_key=false;
                    f.field_Constraint.Const_Name[TOKEN_PRIMARY]="";
                }
            }


        }else if(t.type==TOKEN_NOT){
            next();
            match(TOKEN_NULL);
            if(peek().type==TOKEN_IDENTIFIER){
                for(auto& f:table.fields){
                    if(f.field_Constraint.Const_Name[TOKEN_NOT]==peek().text){
                        f.field_Constraint.not_null=false;
                        f.field_Constraint.Const_Name[TOKEN_PRIMARY]="";
                    }
                }
            }else{
                throw::std::invalid_argument(QString("无约束名，near %1").arg(peek().type).toStdString());

            }
            next();

        }else if(t.type==TOKEN_UNIQUE){
            next();
            if(peek().type==TOKEN_IDENTIFIER){
                for(auto& f:table.fields){
                    if(f.field_Constraint.Const_Name[TOKEN_UNIQUE]==peek().text){
                        f.field_Constraint.Unique_key=false;
                        f.field_Constraint.Const_Name[TOKEN_UNIQUE]="";
                    }
                }
            }else{
                throw::std::invalid_argument(QString("无约束名，near %1").arg(peek().type).toStdString());

            }
            next();

        }else if(t.type==TOKEN_DEFAULT){
            next();
            if(peek().type==TOKEN_IDENTIFIER){
                for(auto& f:table.fields){
                    if(f.field_Constraint.Const_Name[TOKEN_DEFAULT]==peek().text){
                        f.field_Constraint.Const_Name[TOKEN_DEFAULT]="";
                        f.field_Constraint.default_val="";
                    }
                }
            }else{
                throw::std::invalid_argument(QString("无约束名，near %1").arg(peek().type).toStdString());

            }
            next();

        }else if(t.type==TOKEN_AUTO_INCREMENT){
            next();
            if(peek().type==TOKEN_IDENTIFIER){
                for(auto& f:table.fields){
                    if(f.field_Constraint.Const_Name[TOKEN_AUTO_INCREMENT]==peek().text){
                        f.field_Constraint.Auto_increasement=false;
                        f.field_Constraint.Const_Name[TOKEN_AUTO_INCREMENT]="";
                    }
                }
            }else{
                throw::std::invalid_argument(QString("无约束名，near %1").arg(peek().type).toStdString());

            }
            next();

        }
        if(peek().type==TOKEN_COMMA){
            next();
            continue;
        }else{
            match(TOKEN_SEMICOLON);
            continue;
        }
    }
 DDL::saveSchema(table,db.path);
}
//ALTER TABLE 表名 ADD 字段1 类型 约束,
void Parser::paraseAddCol(const QString &sql,DDL::DataBase& db){

    if(db.path.isEmpty()){
        throw::std::invalid_argument("未指定数据库");
    }
    //重置
    tokens.clear();
    pos=0;
    tokens=le.ReadSQL(sql);

    DDL::Table table{};

    QString Tname;
    QString dbPath;

    match(TOKEN_ALTER);
    match(TOKEN_TABLE);

    if(peek().type==TOKEN_IDENTIFIER){
        Tname=peek().text;
        //检验这个数据库是否存在同名表
        dbPath =getDbPathByName(db.name);
        bool exists = isTableExists(dbPath+"/"+db.name+".dbs", Tname);
        if (!exists) {
            throw std::invalid_argument("失败，表不存在！");
        }
        next();
    }else{

        throw::std::invalid_argument(QString("语法错误，near %1").arg(peek().type).toStdString());
    }


    bool isOnePK=false;
    table=DDL::loadSchema(dbPath+"/"+Tname+"/"+Tname+".tbs");
    while(peek().type!=TOKEN_EOF){
         match(TOKEN_ADD);

         QString field_name;
         DDL::FieldType field_type;
         uint16_t length=0;
         DDL::FieldConstraint field_Constraint;


        if(peek().type==TOKEN_IDENTIFIER){
            if(table.hasField(peek().text)){
                throw std::invalid_argument("失败，字段已存在！");
            }else{
                field_name=peek().text;
                next();
            }
        }else{
            qDebug()<<"hello";
            throw::std::invalid_argument(QString("语法错误，near %1").arg(peek().type).toStdString());
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
            qDebug()<<"到这了";
        }else{
            next();
        }

        //解析字段约束
        while(true){

            Token t=peek();

            if(t.type==TOKEN_PRIMARY){

                if(!table.hasPK()){
                    if(!isOnePK){
                        next();
                        match(TOKEN_KEY);
                        field_Constraint.Primary_key=true;
                        field_Constraint.not_null=true;
                        isOnePK=true;
                     }else{
                          throw std::invalid_argument("主键必须唯一");
                    }
                }else{
                     throw std::invalid_argument("主键已存在");
                }

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
            }else if(t.type==TOKEN_AUTO_INCREMENT){
                next();
                field_Constraint.Auto_increasement=true;
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
        }else{
            match(TOKEN_SEMICOLON);
            continue;
        }

    }
    DDL::saveSchema(table,db.path);

}
//MODIFY COLUMN age INT NOT NULL
void Parser:: paraseModifyCol(const QString &sql,DDL::DataBase& db){

    if(db.path.isEmpty()){
        throw::std::invalid_argument("未指定数据库");
    }
    //重置
    tokens.clear();
    pos=0;
    tokens=le.ReadSQL(sql);

    DDL::Table table{};

    QString Tname;
    QString dbPath;

    match(TOKEN_ALTER);
    match(TOKEN_TABLE);

    if(peek().type==TOKEN_IDENTIFIER){
        Tname=peek().text;
        //检验这个数据库是否存在同名表
        dbPath =getDbPathByName(db.name);
        bool exists = isTableExists(dbPath+"/"+db.name+".dbs", Tname);
        if (!exists) {
            throw std::invalid_argument("失败，表不存在！");
        }
        next();
    }else{
        throw::std::invalid_argument(QString("语法错误，near %1").arg(peek().type).toStdString());
    }


    bool isOnePK=false;
    table=DDL::loadSchema(dbPath+"/"+Tname+"/"+Tname+".tbs");
    while(peek().type!=TOKEN_EOF){
        match(TOKEN_MODIFY);

        QString field_name;
        DDL::FieldType field_type;
        uint16_t length=0;
        DDL::FieldConstraint field_Constraint;
        int index=0;

        if(peek().type==TOKEN_IDENTIFIER){
            if(!table.hasField(peek().text)){
                throw std::invalid_argument("失败，字段不存在！");
            }else{
                index=table.getFieldIndex(peek().text);
                table.fields[index].field_name=peek().text;
                next();
            }
        }else{
            throw::std::invalid_argument(QString("语法错误，near %1").arg(peek().type).toStdString());
        }


        field_type=parseFieldType(peek().text);
        table.fields[index].field_type=field_type;

         if(field_type==DDL::FieldType::UNKNOWN){
            throw std::invalid_argument(QString("暂时不支持该字段类型%1").arg(peek().text).toStdString());
        }else{

        }

        if(peek().type==TOKEN_CHAR || peek().type==TOKEN_VARCHAR){
            next();
            match(TOKEN_LPAREN);
            table.fields[index].length = (uint16_t)peek().text.toUShort();
            match(TOKEN_NUMBER);
            match(TOKEN_RPAREN);
            qDebug()<<"到这了";
        }else{
            next();
        }


        //解析字段约束
        while(true){

            Token t=peek();

            if(t.type==TOKEN_PRIMARY){

                if(!table.hasPK()){
                    if(!isOnePK){
                        if(!table.fields[index].field_Constraint.not_null) throw std::invalid_argument("主键必须非空");
                        next();
                        match(TOKEN_KEY);
                        table.fields[index].field_Constraint.Primary_key=true;
                        isOnePK=true;
                    }else{
                        throw std::invalid_argument("主键必须唯一");
                    }
                }else{
                    throw std::invalid_argument("主键已存在");
                }

            }else if(t.type==TOKEN_NOT){
                next();
                match(TOKEN_NULL);
                table.fields[index].field_Constraint.not_null=true;
            }else if(t.type==TOKEN_UNIQUE){
                next();
                table.fields[index].field_Constraint.Unique_key=true;
            }else if(t.type==TOKEN_DEFAULT){
                next();
                table.fields[index].field_Constraint.default_val=peek().text;
                next();
            }else if(t.type==TOKEN_AUTO_INCREMENT){
                next();
                table.fields[index].field_Constraint.Auto_increasement=true;
            }
            else{

                break;
            }

        }

        if(peek().type==TOKEN_COMMA){
            next();
            continue;
        }else{
            match(TOKEN_SEMICOLON);
            continue;
        }

    }
    DDL::saveSchema(table,db.path);

}

void Parser::paraseAddCS(const QString &sql,DDL::DataBase& db){

 qDebug()<<"添加约束";
    if(db.path.isEmpty()){
        throw::std::invalid_argument("未指定数据库");
    }
    //重置
    tokens.clear();
    pos=0;
    tokens=le.ReadSQL(sql);

    DDL::Table table{};

    QString Tname;
    QString dbPath;

    match(TOKEN_ALTER);
    match(TOKEN_TABLE);

    if(peek().type==TOKEN_IDENTIFIER){
        Tname=peek().text;
        //检验这个数据库是否存在同名表
        dbPath =getDbPathByName(db.name);
        bool exists = isTableExists(dbPath+"/"+db.name+".dbs", Tname);
        if (!exists) {
            throw std::invalid_argument("失败，表不存在！");
        }
        next();
    }else{

        throw::std::invalid_argument(QString("语法错误，near %1").arg(peek().type).toStdString());
    }


    bool isOnePK=false;
    table=DDL::loadSchema(dbPath+"/"+Tname+"/"+Tname+".tbs");

     while(peek().type!=TOKEN_EOF){

        match(TOKEN_ADD);
        match(TOKEN_CONSTRAINT);

         QString field_name;
         DDL::FieldType field_type;
         uint16_t length=0;
         int Ccount=0;
         DDL::FieldConstraint field_Constraint;
         QString Cname;
             if(peek().type==TOKEN_IDENTIFIER){
                 Cname=peek().text;
                 next();
             }else{
                  throw::std::invalid_argument(QString("语法错误，无约束名，near %1").arg(peek().type).toStdString());
             }
             Token t=peek();

             if(t.type==TOKEN_PRIMARY){
                 if(!table.hasPK()){
                     if(!isOnePK){
                         next();
                         match(TOKEN_KEY);
                         field_Constraint.Primary_key=true;
                         isOnePK=true;
                     }else{
                         throw std::invalid_argument("主键必须唯一");
                     }
                 }else{
                     throw std::invalid_argument("主键已存在");
                 }
             }else if(t.type==TOKEN_NOT){
                 next();
                 match(TOKEN_NULL);
                 throw std::invalid_argument(QString("语法错误不支持NOT NULL: near %1").arg(peek().text).toStdString());

             }else if(t.type==TOKEN_UNIQUE){
                 next();
                 field_Constraint.Unique_key=true;

             }else if(t.type==TOKEN_DEFAULT){
                 next();
                 field_Constraint.default_val=peek().text;

                 next();
             }else if(t.type==TOKEN_AUTO_INCREMENT){
                 next();
                 field_Constraint.Auto_increasement=true;

             }else if(t.type==TOKEN_FOREIGN){
                 QString fN = hasForeign(table,db);
                 int i = table.getFieldIndex(fN);
                 table.fields[i].field_Constraint.Const_Name[TOKEN_FOREIGN]=Cname;

                 if(peek().type==TOKEN_COMMA){
                     next();
                     continue;
                 }else{
                     match(TOKEN_SEMICOLON);
                     continue;
                 }
             }else{
                 throw std::invalid_argument(QString("语法错误,无约束: near %1").arg(peek().text).toStdString());
             }
             match(TOKEN_LPAREN);
             bool isHaveField=false;
             if(peek().type==TOKEN_IDENTIFIER){
                 for(int i=0;i<table.fields.size();++i){
                     if(table.fields[i].field_name==peek().text){
                         qDebug()<<table.fields[i].field_name;
                         // 合并约束，保留原来的，只新增表级约束
                         if (field_Constraint.Primary_key)
                             table.fields[i].field_Constraint.Primary_key = true;
                         if (field_Constraint.Unique_key)
                             qDebug()<<"更新"<<table.fields[i].field_name<<"UNIQUE约束";
                         table.fields[i].field_Constraint.Unique_key = true;
                         if (!field_Constraint.default_val.isEmpty())
                             table.fields[i].field_Constraint.default_val = field_Constraint.default_val;
                         if (field_Constraint.Auto_increasement)
                             table.fields[i].field_Constraint.Auto_increasement = true;
                         if(field_Constraint.Foreign_key)
                             table.fields[i].field_Constraint.Foreign_key = true;


                         table.fields[i].field_Constraint.Const_Name[t.type]=Cname;
                         isHaveField=true;
                         break;
                     }
                 }
                 if(!isHaveField){

                     throw std::invalid_argument(QString("无该字段: near %1").arg(peek().text).toStdString());
                 }
             }else{
                 throw std::invalid_argument(QString("语法错误: near %1").arg(peek().text).toStdString());
             }
             next();
             match(TOKEN_RPAREN);

         if(peek().type==TOKEN_COMMA){
             next();
             continue;
         }else{
             match(TOKEN_SEMICOLON);
             continue;
         }
     }
    DDL::saveSchema(table,db.path);
}

// ==============================================
// DML 解析实现
// ==============================================

// 解析 INSERT
// 支持：
//   INSERT INTO table VALUES (...);
//   INSERT INTO table (col1, col2) VALUES (...);
//   INSERT INTO table VALUES (...), (...), (...);
InsertStatement Parser::parseInsert(const QString& sql)
{
    tokens.clear();
    pos = 0;
    tokens = le.ReadSQL(sql);

    InsertStatement stmt;

    // INSERT
    match(TOKEN_INSERT);

    // INTO
    match(TOKEN_INTO);

    // 表名
    if (peek().type == TOKEN_IDENTIFIER) {
        stmt.tableName = peek().text;
        next();
    } else {
        throw std::invalid_argument("语法错误：缺少表名");
    }

    // 检查是否有列名
    if (peek().type == TOKEN_LPAREN) {
        next(); // 跳过 (

        // 解析列名列表
        while (peek().type != TOKEN_EOF) {
            if (peek().type == TOKEN_IDENTIFIER) {
                stmt.columns.append(peek().text);
                next();
            } else {
                throw std::invalid_argument(QString("语法错误：期望列名 near %1").arg(peek().text).toStdString());
            }

            if (peek().type == TOKEN_COMMA) {
                next();
                continue;
            } else if (peek().type == TOKEN_RPAREN) {
                next();
                break;
            } else {
                throw std::invalid_argument(QString("语法错误：期望 , 或 ) near %1").arg(peek().text).toStdString());
            }
        }
    }

    // VALUES
    match(TOKEN_VALUES);

    // 循环解析每一行 (...)，(...)，(...)
    while (true) {
        // 解析一行：(
        match(TOKEN_LPAREN);

        // 解析这一行的值
        QVector<QString> row;
        while (peek().type != TOKEN_EOF) {
            if (peek().type == TOKEN_NUMBER) {
                row.append(peek().text);
                next();
            } else if (peek().type == TOKEN_STRING) {
                row.append(peek().text);
                next();
            } else if (peek().type == TOKEN_IDENTIFIER) {
                // 处理 NULL 或其他标识符
                if (peek().text.toUpper() == "NULL") {
                    row.append("");
                    next();
                } else {
                    row.append(peek().text);
                    next();
                }
            } else {
                throw std::invalid_argument(QString("语法错误：期望值 near %1").arg(peek().text).toStdString());
            }

            if (peek().type == TOKEN_COMMA) {
                next();
                continue;
            } else if (peek().type == TOKEN_RPAREN) {
                next();
                break;
            } else {
                throw std::invalid_argument(QString("语法错误：期望 , 或 ) near %1").arg(peek().text).toStdString());
            }
        }

        // 将这一行加入 rows
        stmt.rows.append(row);

        // 检查是还有下一行：应该有 , 后跟 (
        if (peek().type == TOKEN_COMMA) {
            next();
            if (peek().type == TOKEN_LPAREN) {
                continue;  // 继续解析下一行
            } else {
                throw std::invalid_argument(QString("语法错误：期望 ( after , near %1").arg(peek().text).toStdString());
            }
        } else if (peek().type == TOKEN_SEMICOLON || peek().type == TOKEN_EOF) {
            break;  // 结束
        } else {
            throw std::invalid_argument(QString("语法错误：期望 , 或 ; near %1").arg(peek().text).toStdString());
        }
    }

    // 分号（可选）
    if (peek().type == TOKEN_SEMICOLON) {
        next();
    }

    return stmt;
}

// 解析 UPDATE
// 支持：UPDATE table SET col1 = value1, col2 = value2 WHERE col = value;
UpdateStatement Parser::parseUpdate(const QString& sql)
{
    tokens.clear();
    pos = 0;
    tokens = le.ReadSQL(sql);

    UpdateStatement stmt;

    // UPDATE
    match(TOKEN_UPDATE);

    // 表名
    if (peek().type == TOKEN_IDENTIFIER) {
        stmt.tableName = peek().text;
        next();
    } else {
        throw std::invalid_argument("语法错误：缺少表名");
    }

    // SET
    match(TOKEN_SET);

    // 解析 SET 部分：col = value [, col = value]
    while (peek().type != TOKEN_EOF) {
        if (peek().type == TOKEN_IDENTIFIER) {
            QString colName = peek().text;
            next();

            match(TOKEN_EQUAL);

            QString value;
            if (peek().type == TOKEN_NUMBER) {
                value = peek().text;
                next();
            } else if (peek().type == TOKEN_STRING) {
                value = peek().text;
                next();
            } else if (peek().type == TOKEN_IDENTIFIER) {
                if (peek().text.toUpper() == "NULL") {
                    value = "";
                    next();
                } else {
                    value = peek().text;
                    next();
                }
            } else {
                throw std::invalid_argument(QString("语法错误：期望值 near %1").arg(peek().text).toStdString());
            }

            stmt.setMap[colName] = value;
        } else {
            throw std::invalid_argument(QString("语法错误：期望列名 near %1").arg(peek().text).toStdString());
        }

        if (peek().type == TOKEN_COMMA) {
            next();
            continue;
        } else if (peek().type == TOKEN_WHERE || peek().type == TOKEN_EOF) {
            break;
        } else {
            throw std::invalid_argument(QString("语法错误：期望 , 或 WHERE near %1").arg(peek().text).toStdString());
        }
    }

    // WHERE
    if (peek().type == TOKEN_WHERE) {
        next();

        // WHERE 列名
        if (peek().type == TOKEN_IDENTIFIER) {
            stmt.whereColumn = peek().text;
            next();
        } else {
            throw std::invalid_argument("语法错误：WHERE 后缺少列名");
        }

        // =
        match(TOKEN_EQUAL);

        // WHERE 值
        if (peek().type == TOKEN_NUMBER) {
            stmt.whereValue = peek().text;
            next();
        } else if (peek().type == TOKEN_STRING) {
            stmt.whereValue = peek().text;
            next();
        } else if (peek().type == TOKEN_IDENTIFIER) {
            if (peek().text.toUpper() == "NULL") {
                stmt.whereValue = "";
                next();
            } else {
                stmt.whereValue = peek().text;
                next();
            }
        } else {
            throw std::invalid_argument(QString("语法错误：期望值 near %1").arg(peek().text).toStdString());
        }
    } else {
        throw std::invalid_argument("UPDATE 语句必须包含 WHERE 条件");
    }

    // 分号（可选）
    if (peek().type == TOKEN_SEMICOLON) {
        next();
    }

    return stmt;
}

// 解析 DELETE
// 支持：DELETE FROM table WHERE col = value;
DeleteStatement Parser::parseDelete(const QString& sql)
{
    tokens.clear();
    pos = 0;
    tokens = le.ReadSQL(sql);

    DeleteStatement stmt;

    // DELETE
    match(TOKEN_DELETE);

    // FROM
    match(TOKEN_FROM);

    // 表名
    if (peek().type == TOKEN_IDENTIFIER) {
        stmt.tableName = peek().text;
        next();
    } else {
        throw std::invalid_argument("语法错误：缺少表名");
    }

    // WHERE
    if (peek().type == TOKEN_WHERE) {
        next();

        // WHERE 列名
        if (peek().type == TOKEN_IDENTIFIER) {
            stmt.whereColumn = peek().text;
            next();
        } else {
            throw std::invalid_argument("语法错误：WHERE 后缺少列名");
        }

        // =
        match(TOKEN_EQUAL);

        // WHERE 值
        if (peek().type == TOKEN_NUMBER) {
            stmt.whereValue = peek().text;
            next();
        } else if (peek().type == TOKEN_STRING) {
            stmt.whereValue = peek().text;
            next();
        } else if (peek().type == TOKEN_IDENTIFIER) {
            if (peek().text.toUpper() == "NULL") {
                stmt.whereValue = "";
                next();
            } else {
                stmt.whereValue = peek().text;
                next();
            }
        } else {
            throw std::invalid_argument(QString("语法错误：期望值 near %1").arg(peek().text).toStdString());
        }
    } else {
        throw std::invalid_argument("DELETE 语句必须包含 WHERE 条件");
    }

    // 分号（可选）
    if (peek().type == TOKEN_SEMICOLON) {
        next();
    }

    return stmt;
}

// ==============================================
// SELECT 解析（仅支持 SELECT * FROM 表名）
// ==============================================
SelectStatement Parser::parseSelect(const QString& sql)
{
    tokens.clear();
    pos = 0;
    tokens = le.ReadSQL(sql);

    SelectStatement stmt;

    // SELECT
    match(TOKEN_SELECT);

    // *（只支持通配符）
    if (peek().type == TOKEN_STAR) {
        next();
    } else {
        throw std::invalid_argument("当前仅支持 SELECT * 语法");
    }

    // FROM
    match(TOKEN_FROM);

    // 表名
    if (peek().type == TOKEN_IDENTIFIER) {
        stmt.tableName = peek().text;
        next();
    } else {
        throw std::invalid_argument("语法错误：期望表名");
    }

    return stmt;
}


