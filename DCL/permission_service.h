#ifndef PERMISSION_SERVICE_H
#define PERMISSION_SERVICE_H

#include "dcl_types.h"
#include "../DDL/DDL.h"
#include <QString>

namespace DCL {

class PermissionService {
public:
    explicit PermissionService(const QString& rootPath = "../../dataDB");

    bool ensureStorage(QString& error) const;

    bool checkPermission(const SessionContext& session,
                         TableAction action,
                         const QString& databaseName,
                         const QString& tableName,
                         QString& error) const;

    bool grantPermission(const SessionContext& session,
                         const QString& username,
                         TableAction action,
                         const QString& databaseName,
                         const QString& tableName,
                         QString& error);

    bool revokePermission(const SessionContext& session,
                          const QString& username,
                          TableAction action,
                          const QString& databaseName,
                          const QString& tableName,
                          QString& error);

    bool removePermissionsForUser(const QString& username, QString& error);

private:
    QString sysDbPath() const;
    DDL::Table permissionsTable() const;

    bool hasExplicitPermission(const QString& username,
                               TableAction action,
                               const QString& databaseName,
                               const QString& tableName) const;

    QVector<QVector<QString>> loadPermissions() const;
    void savePermissions(const QVector<QVector<QString>>& permissions) const;
    QString actionToString(TableAction action) const;

    QString m_rootPath;
};

} // namespace DCL

#endif // PERMISSION_SERVICE_H
