#include "Log.h"
#include "DCL/dcl_facade.h"
#include <QDir>



void Log::writeToLog(QString& dbPath,const QString& userName,QString sql){


    QString logDir = dbPath + "/logs";
    QDir().mkpath(logDir); // 确保目录存在

    QString date = QDateTime::currentDateTime().toString("yyyy-MM-dd");
    QString logFile = logDir + "/" + date + ".log";


    QFile file(logFile);

    if (!file.open(QIODevice::Append | QIODevice::Text)) return;

    //文本写入
    QTextStream out(&file);

    QString time = QDateTime::currentDateTime().toString("HH:mm:ss");
    QString log=QString("[%1]-%2:%3").arg(time).arg(userName).arg(sql);


    out<<log<<'\n';

    file.close();
}
