#ifndef DATABASEEXPLORER_H
#define DATABASEEXPLORER_H

#include <QWidget>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>
#include <QString>
#include <QStringList>

#include "DCL/user_repository.h"
#include "DCL/permission_service.h"

namespace DCL {
class DclFacade;
}

class DatabaseExplorer : public QWidget
{
    Q_OBJECT

public:
    explicit DatabaseExplorer(DCL::DclFacade* facade,
                              const QString& rootPath,
                              QWidget *parent = nullptr);

signals:
    void viewTableRequested(const QString& dbName, const QString& tableName);
    void viewDataRequested(const QString& dbName, const QString& tableName);
    void createTableRequested(const QString& dbName);
    void deleteTableRequested(const QString& dbName, const QString& tableName);
    void modifyTableRequested(const QString& dbName, const QString& tableName);
    void terminalMessage(const QString& message);

public slots:
    void refreshTree();

private slots:
    void onTreeRightClicked(const QPoint& pos);

private:
    void displayDB();
    void refreshDBTreeWithState();
    QStringList saveExpandedPaths(QTreeWidgetItem* item, const QString& parentPath);
    void restoreExpandedPaths(QTreeWidgetItem* item, const QString& parentPath,
                              const QStringList& paths);

    QTreeWidget* m_treeWidget;
    DCL::DclFacade* m_dclFacade;
    DCL::UserRepository m_userRepo;
    DCL::PermissionService m_permService;
    QString m_rootPath;
};

#endif // DATABASEEXPLORER_H
