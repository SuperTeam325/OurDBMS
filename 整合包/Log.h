#ifndef LOG_H
#define LOG_H
#include "DCL/dcl_facade.h"
#include <QString>
class Log{


public :
    static void writeToLog(QString& dbPath,const QString& userName,QString sql);


private :
    DCL::DclFacade* dclFacade;





};



#endif // LOG_H
