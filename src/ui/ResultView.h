#ifndef RESULTVIEW_H
#define RESULTVIEW_H

#include <QWidget>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QPushButton>
#include "DDL.h"

class Parser;

class ResultView : public QWidget
{
    Q_OBJECT

public:
    explicit ResultView(Parser* parser, QWidget *parent = nullptr);

    void showTableSchema(const QString& dbName, const QString& tableName);
    void showTableData(const QString& dbName, const QString& tableName);
    void clear();

signals:
    void backToEditor();

private:
    void clearLayout();

    Parser* m_parser;
    QVBoxLayout* m_mainLayout;
};

#endif // RESULTVIEW_H
