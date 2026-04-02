#ifndef AST_H
#define AST_H
#include <QString>
#include <QList>

struct FieldConstraint {
    bool not_null = false;
    bool primary_key = false;
    bool unique = false;
    bool auto_increment = false;
    QString default_val;
};

enum FieldType {
    TYPE_INT,
    TYPE_FLOAT,
    TYPE_CHAR,
    TYPE_VARCHAR,
    TYPE_UNKNOWN
};

struct Field {
    QString name;
    FieldType type;
    int length = 0;
    FieldConstraint constraint;

    Field(QString n = "", FieldType t = TYPE_UNKNOWN, int len = 0)
        : name(n), type(t), length(len) {}
};

struct Table {
    QString name;
    QList<Field> fields;
};

#endif // AST_H
