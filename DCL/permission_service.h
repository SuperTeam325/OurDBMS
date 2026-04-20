#ifndef PERMISSION_SERVICE_H
#define PERMISSION_SERVICE_H

#include "dcl_types.h"
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

private:
    QString permissionsFilePath() const;
    bool hasExplicitPermission(const QString& username,
                               TableAction action,
                               const QString& databaseName,
                               const QString& tableName) const;
    QString actionToString(TableAction action) const;

    QString m_rootPath;
};

} // namespace DCL

#endif // PERMISSION_SERVICE_H
