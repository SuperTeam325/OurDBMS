#include "src/ui/ResultView.h"
#include "Parser.h"
#include "DML.h"
#include <QHeaderView>
#include <QLabel>

ResultView::ResultView(Parser* parser, QWidget *parent)
    : QWidget(parent)
    , m_parser(parser)
{
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(20, 20, 20, 30);
    m_mainLayout->setSpacing(15);
}

void ResultView::clearLayout()
{
    QLayoutItem* child;
    while ((child = m_mainLayout->takeAt(0)) != nullptr) {
        if (child->widget()) {
            child->widget()->deleteLater();
        }
        delete child;
    }
}

void ResultView::showTableSchema(const QString& dbName, const QString& tableName)
{
    clearLayout();

    QString dbPath = m_parser->getDbPathByName(dbName);
    QString tbsPath = dbPath + "/" + tableName + "/" + tableName + ".tbs";
    DDL::Table table = DDL::loadSchema(tbsPath);

    QTableWidget* tw = new QTableWidget;
    tw->setColumnCount(4);
    tw->setHorizontalHeaderLabels({
        QString::fromUtf8("字段名"),
        QString::fromUtf8("字段类型"),
        QString::fromUtf8("长度"),
        QString::fromUtf8("约束")
    });

    tw->setShowGrid(true);
    tw->setAlternatingRowColors(true);
    tw->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    tw->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    tw->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

    QVector<TokenType> CSType = {TOKEN_NOT, TOKEN_DEFAULT, TOKEN_PRIMARY,
                                 TOKEN_UNIQUE, TOKEN_AUTO_INCREMENT, TOKEN_FOREIGN};
    for (const DDL::Field& f : table.fields) {
        int r = tw->rowCount();
        tw->insertRow(r);
        tw->setItem(r, 0, new QTableWidgetItem(f.field_name));
        tw->setItem(r, 1, new QTableWidgetItem(DDL::fieldTypeToString(f.field_type)));
        tw->setItem(r, 2, new QTableWidgetItem(QString::number(f.length)));

        QString cons;
        for (auto c : CSType) {
            if (!f.field_Constraint.Const_Name[c].isEmpty())
                cons += f.field_Constraint.Const_Name[c] + "("
                        + f.field_Constraint.toString(c) + ")\n";
        }
        tw->setItem(r, 3, new QTableWidgetItem(cons.trimmed()));

        for (int c = 0; c < 4; c++)
            tw->item(r, c)->setTextAlignment(Qt::AlignCenter);
    }

    QPushButton* btnBack = new QPushButton(QString::fromUtf8("返回主页"));
    btnBack->setMinimumWidth(200);

    connect(btnBack, &QPushButton::clicked, this, &ResultView::backToEditor);

    m_mainLayout->addWidget(tw);
    m_mainLayout->addWidget(btnBack, 0, Qt::AlignCenter);
}

void ResultView::showTableData(const QString& dbName, const QString& tableName)
{
    clearLayout();

    QString dbPath = m_parser->getDbPathByName(dbName);
    QString tbsPath = dbPath + "/" + tableName + "/" + tableName + ".tbs";
    DDL::Table table = DDL::loadSchema(tbsPath);

    DDL::DataBase db;
    db.name = dbName;
    db.path = dbPath;

    QVector<QVector<QString>> tableData = DML::loadTableRows(db, table);

    QTableWidget* tw = new QTableWidget;
    int columnCount = table.fields.size();
    int rowCount = tableData.size();

    tw->setColumnCount(columnCount);
    tw->setRowCount(rowCount);

    QStringList headers;
    for (const DDL::Field& f : table.fields) {
        headers << f.field_name;
    }
    tw->setHorizontalHeaderLabels(headers);

    tw->setShowGrid(true);
    tw->setAlternatingRowColors(true);
    tw->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    tw->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    tw->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

    for (int row = 0; row < tableData.size(); row++) {
        const QVector<QString>& rowData = tableData[row];
        for (int col = 0; col < rowData.size(); col++) {
            tw->setItem(row, col, new QTableWidgetItem(rowData[col]));
            tw->item(row, col)->setTextAlignment(Qt::AlignCenter);
        }
    }

    if (tableData.isEmpty()) {
        tw->setRowCount(1);
        tw->setItem(0, 0, new QTableWidgetItem(QString::fromUtf8("(表数据为空)")));
        tw->item(0, 0)->setTextAlignment(Qt::AlignCenter);
    }

    QPushButton* btnBack = new QPushButton(QString::fromUtf8("返回主页"));
    btnBack->setMinimumWidth(200);

    connect(btnBack, &QPushButton::clicked, this, &ResultView::backToEditor);

    m_mainLayout->addWidget(tw);
    m_mainLayout->addWidget(btnBack, 0, Qt::AlignCenter);
}

void ResultView::clear()
{
    clearLayout();
}
