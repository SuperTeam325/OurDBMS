#include "src/ui/DatabaseExplorer.h"
#include "DCL/dcl_facade.h"
#include "DCL/session_manager.h"
#include "DCL/dcl_types.h"
#include "Parser.h"
#include <QDir>
#include <QMenu>
#include <QDebug>

DatabaseExplorer::DatabaseExplorer(DCL::DclFacade* facade,
                                   const QString& rootPath,
                                   QWidget *parent)
    : QWidget(parent)
    , m_dclFacade(facade)
    , m_userRepo(rootPath + "/dataDB")
    , m_permService(rootPath + "/dataDB")
    , m_rootPath(rootPath)
{
    m_treeWidget = new QTreeWidget(this);
    m_treeWidget->setHeaderLabel(QString::fromUtf8("数据库导航"));
    m_treeWidget->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(m_treeWidget, &QTreeWidget::customContextMenuRequested,
            this, &DatabaseExplorer::onTreeRightClicked);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_treeWidget);

    displayDB();
}

void DatabaseExplorer::displayDB()
{
    m_treeWidget->clear();

    QString path = m_rootPath + "/dataDB";
    QDir rootDir(path);

    QFileInfoList dbDirList = rootDir.entryInfoList(
        QDir::Dirs | QDir::NoDotAndDotDot);

    QTreeWidgetItem* normalDbGroup = new QTreeWidgetItem(m_treeWidget);
    normalDbGroup->setText(0, QString::fromUtf8("数据库"));

    QTreeWidgetItem* systemDbGroup = new QTreeWidgetItem(m_treeWidget);
    systemDbGroup->setText(0, QString::fromUtf8("系统数据库"));

    QTreeWidgetItem* UserGroup = new QTreeWidgetItem(m_treeWidget);
    UserGroup->setText(0, QString::fromUtf8("用户"));

    for (QFileInfo dbInfo : dbDirList) {
        QString dbName = dbInfo.fileName();
        QTreeWidgetItem* targetGroup;

        if (dbName == "sys") {
            targetGroup = systemDbGroup;
        } else {
            targetGroup = normalDbGroup;
        }

        QTreeWidgetItem* dbItem = new QTreeWidgetItem(targetGroup);
        dbItem->setText(0, dbName);

        QTreeWidgetItem* tableGroupItem = new QTreeWidgetItem(dbItem);
        tableGroupItem->setText(0, QString::fromUtf8("表"));

        QDir dbFolder(dbInfo.absoluteFilePath());
        QFileInfoList tableDirList = dbFolder.entryInfoList(
            QDir::Dirs | QDir::NoDotAndDotDot);

        for (QFileInfo tableInfo : tableDirList) {
            QString tableName = tableInfo.fileName();
            QTreeWidgetItem* tableItem = new QTreeWidgetItem(tableGroupItem);
            tableItem->setText(0, tableName);

            QString tbsPath = tableInfo.absoluteFilePath() + "/" + tableName + ".tbs";
            DDL::Table table = DDL::loadSchema(tbsPath);

            QTreeWidgetItem* colGroupItem = new QTreeWidgetItem(tableItem);
            colGroupItem->setText(0, QString::fromUtf8("列"));
            for (const DDL::Field& f : table.fields) {
                QString fieldText = QString("%1 (%2, %3)")
                    .arg(f.field_name)
                    .arg(DDL::fieldTypeToString(f.field_type))
                    .arg(f.length);
                QTreeWidgetItem* fieldItem = new QTreeWidgetItem(colGroupItem);
                fieldItem->setText(0, fieldText);
            }

            QTreeWidgetItem* ConstGroupItem = new QTreeWidgetItem(tableItem);
            ConstGroupItem->setText(0, QString::fromUtf8("约束"));
            QVector<TokenType> CSType = {TOKEN_NOT, TOKEN_DEFAULT, TOKEN_PRIMARY,
                                         TOKEN_UNIQUE, TOKEN_AUTO_INCREMENT, TOKEN_FOREIGN};
            for (const DDL::Field& f : table.fields) {
                for (auto cst : CSType) {
                    if (!f.field_Constraint.Const_Name[cst].isEmpty()) {
                        QString CSText = QString("%1(%2)")
                            .arg(f.field_Constraint.Const_Name[cst])
                            .arg(f.field_Constraint.toString(cst));
                        QTreeWidgetItem* CsItem = new QTreeWidgetItem(ConstGroupItem);
                        CsItem->setText(0, CSText);
                    }
                }
            }
        }
    }

    // Load user info
    QVector<QVector<QString>> users = DDL::loadTableData(
        m_userRepo.usersTable(), path + "/sys");
    QVector<QVector<QString>> permission = DDL::loadTableData(
        m_permService.permissionsTable(), path + "/sys");

    for (int i = 0; i < users.size(); ++i) {
        QTreeWidgetItem* NameItem = new QTreeWidgetItem(UserGroup);
        NameItem->setText(0, users[i][0]);
        QTreeWidgetItem* PItem = new QTreeWidgetItem(NameItem);
        PItem->setText(0, QString::fromUtf8("授权"));

        if (users[i][3] == "1") {
            QTreeWidgetItem* item = new QTreeWidgetItem(PItem);
            item->setText(0, "ALL PRIVILEGES");
        }
        for (int j = 0; j < permission.size(); ++j) {
            if (users[i][0] == permission[j][0]) {
                QTreeWidgetItem* item1 = new QTreeWidgetItem(PItem);
                item1->setText(0, QString(permission[j][3] + "(%1,%2)")
                    .arg(permission[j][1]).arg(permission[j][2]));
            }
        }
    }
}

QStringList DatabaseExplorer::saveExpandedPaths(QTreeWidgetItem* item,
                                                 const QString& parentPath)
{
    QStringList paths;
    if (!item) return paths;

    QString currentPath = parentPath.isEmpty() ? item->text(0)
                                               : parentPath + "/" + item->text(0);

    if (item->isExpanded()) {
        paths.append(currentPath);
    }

    for (int i = 0; i < item->childCount(); ++i) {
        paths += saveExpandedPaths(item->child(i), currentPath);
    }
    return paths;
}

void DatabaseExplorer::restoreExpandedPaths(QTreeWidgetItem* item,
                                             const QString& parentPath,
                                             const QStringList& paths)
{
    if (!item) return;

    QString currentPath = parentPath.isEmpty() ? item->text(0)
                                               : parentPath + "/" + item->text(0);

    if (paths.contains(currentPath)) {
        item->setExpanded(true);
    }

    for (int i = 0; i < item->childCount(); ++i) {
        restoreExpandedPaths(item->child(i), currentPath, paths);
    }
}

void DatabaseExplorer::refreshDBTreeWithState()
{
    QStringList expandedPaths;
    for (int i = 0; i < m_treeWidget->topLevelItemCount(); ++i) {
        expandedPaths += saveExpandedPaths(m_treeWidget->topLevelItem(i), "");
    }

    displayDB();

    for (int i = 0; i < m_treeWidget->topLevelItemCount(); ++i) {
        restoreExpandedPaths(m_treeWidget->topLevelItem(i), "", expandedPaths);
    }
}

void DatabaseExplorer::refreshTree()
{
    refreshDBTreeWithState();
}

void DatabaseExplorer::onTreeRightClicked(const QPoint& pos)
{
    QTreeWidgetItem* item = m_treeWidget->itemAt(pos);
    if (!item) return;

    QMenu menu(this);

    QString now = item->text(0);
    QTreeWidgetItem* parent = item->parent();
    QString pText = parent ? parent->text(0) : "";

    bool isDatabaseNode = (pText == QString::fromUtf8("数据库"));
    bool isTableNode = (pText == QString::fromUtf8("表"));
    bool isColOrConstNode = (pText == QString::fromUtf8("列")
                             || pText == QString::fromUtf8("约束"));

    if (isDatabaseNode) {
        if (item->text(0) != "sys") {
            QString dbName = item->text(0);
            menu.addAction(QString::fromUtf8("新建表"), this, [this, dbName]() {
                emit terminalMessage(QString::fromUtf8("右键 → 新建表（数据库：") + dbName + ")");
                emit createTableRequested(dbName);
            });
        }
        menu.addSeparator();
        menu.addAction(QString::fromUtf8("刷新"), this, &DatabaseExplorer::refreshTree);
    }
    else if (isTableNode) {
        QString tableName = item->text(0);
        QTreeWidgetItem* dbParent = item->parent()->parent();
        QString dbName = dbParent ? dbParent->text(0) : "";

        menu.addAction(QString::fromUtf8("查看表"), this, [this, dbName, tableName]() {
            emit viewTableRequested(dbName, tableName);
        });
        menu.addAction(QString::fromUtf8("查看数据"), this, [this, dbName, tableName]() {
            emit viewDataRequested(dbName, tableName);
        });
        if (dbName != "sys") {
            menu.addAction(QString::fromUtf8("修改表结构"), this, [this, dbName, tableName]() {
                emit modifyTableRequested(dbName, tableName);
            });
            menu.addAction(QString::fromUtf8("删除表"), this, [this, dbName, tableName]() {
                emit deleteTableRequested(dbName, tableName);
            });
        }
        menu.addSeparator();
        menu.addAction(QString::fromUtf8("刷新"), this, &DatabaseExplorer::refreshTree);
    }
    else if (isColOrConstNode) {
        menu.addAction(QString::fromUtf8("查看详情"));
        menu.addSeparator();
        menu.addAction(QString::fromUtf8("刷新"), this, &DatabaseExplorer::refreshTree);
    }
    else {
        menu.addAction(QString::fromUtf8("刷新"), this, &DatabaseExplorer::refreshTree);
    }

    menu.exec(m_treeWidget->viewport()->mapToGlobal(pos));
}
