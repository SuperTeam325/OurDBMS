#ifndef DQL_H
#define DQL_H

#include "DDL.h"
#include <QString>

class DQL
{
public:
    static QString executeQuery(const DDL::DataBase &db, const QString &sql);
};

#endif // DQL_H
