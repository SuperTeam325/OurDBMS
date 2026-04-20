#include "permission_service.h"
#include <QDir>
#include <QFile>

namespace DCL {

PermissionService::PermissionService(const QString& rootPath)
    : m_rootPath(rootPath) {}

bool PermissionService::ensureStorage(QString& error) const
{
    const QString sysPath = sysDbPath();
    QDir dir;
    if (!dir.exists(sysPath) && !dir.mkpath(sysPath)) {
        error = "无法创建系统库目录";
        return false;
    }

    const QString dbsPath = sysPath + "/sys.dbs";
    QFile dbsFile(dbsPath);
    if (!dbsFile.exists()) {
        if (!dbsFile.open(QIODevice::WriteOnly)) {
            error = "无法创建系统库索引文件";
            return false;
        }
        dbsFile.close();
    }

    QString dbsPathRef = dbsPath;
    const QStringList tableNames = DDL::readFromDbs(dbsPathRef);
    if (!tableNames.contains("permissions")) {
        DDL::DataBase db;
        db.name = "sys";
        db.path = sysPath;
        DDL::Table table = permissionsTable();
        DDL::writeToDbs(db, table);
        DDL::saveSchema(table, db.path);
    }

    const QString permissionsDbfPath = sysPath + "/permissions/permissions.dbf";
    QFile permissionsDbf(permissionsDbfPath);
    if (!permissionsDbf.exists()) {
        DDL::saveTableData(permissionsTable(), {}, sysPath);
    }

    return true;
}

bool PermissionService::checkPermission(const SessionContext& session,
                                        TableAction action,
                                        const QString& databaseName,
                                        const QString& tableName,
                                        QString& error) const
{
    if (!session.isLoggedIn) {
        error = "未登录，禁止执行 SQL";
        return false;
    }

    if (session.isAdmin) {
        return true;
    }

    if (action == TableAction::Unknown) {
        return true;
    }

    if (action == TableAction::Select && !databaseName.isEmpty() && databaseName == session.currentDatabase) {
        return true;
    }

    if (hasExplicitPermission(session.username, action, databaseName, tableName)) {
        return true;
    }

    error = QString("权限不足：用户 %1 无法执行 %2").arg(session.username, actionToString(action));
    return false;
}

bool PermissionService::grantPermission(const SessionContext& session,
                                        const QString& username,
                                        TableAction action,
                                        const QString& databaseName,
                                        const QString& tableName,
                                        QString& error)
{
    if (!session.isLoggedIn || !session.isAdmin) {
        error = "只有管理员可以执行 GRANT";
        return false;
    }

    if (username.trimmed().isEmpty() || databaseName.trimmed().isEmpty() || tableName.trimmed().isEmpty() || action == TableAction::Unknown) {
        error = "GRANT 参数不完整";
        return false;
    }

    QVector<QVector<QString>> permissions = loadPermissions();
    const QString actionText = actionToString(action);
    for (const auto& row : permissions) {
        if (row.size() >= 4 && row[0] == username && row[1] == databaseName && row[2] == tableName && row[3] == actionText) {
            return true;
        }
    }

    permissions.append({username, databaseName, tableName, actionText});
    savePermissions(permissions);
    return true;
}

bool PermissionService::revokePermission(const SessionContext& session,
                                         const QString& username,
                                         TableAction action,
                                         const QString& databaseName,
                                         const QString& tableName,
                                         QString& error)
{
    if (!session.isLoggedIn || !session.isAdmin) {
        error = "只有管理员可以执行 REVOKE";
        return false;
    }

    if (username.trimmed().isEmpty() || databaseName.trimmed().isEmpty() || tableName.trimmed().isEmpty() || action == TableAction::Unknown) {
        error = "REVOKE 参数不完整";
        return false;
    }

    QVector<QVector<QString>> permissions = loadPermissions();
    const QString actionText = actionToString(action);
    QVector<QVector<QString>> filtered;
    bool removed = false;
    for (const auto& row : permissions) {
        if (row.size() >= 4 && row[0] == username && row[1] == databaseName && row[2] == tableName && row[3] == actionText) {
            removed = true;
            continue;
        }
        filtered.append(row);
    }

    if (!removed) {
        return true;
    }

    savePermissions(filtered);
    return true;
}

bool PermissionService::removePermissionsForUser(const QString& username, QString& error)
{
    const QString normalizedUsername = username.trimmed();
    if (normalizedUsername.isEmpty()) {
        error = "用户名不能为空";
        return false;
    }

    QVector<QVector<QString>> permissions = loadPermissions();
    QVector<QVector<QString>> filtered;
    for (const auto& row : permissions) {
        if (row.size() >= 4 && row[0] == normalizedUsername) {
            continue;
        }
        filtered.append(row);
    }

    savePermissions(filtered);
    return true;
}

QString PermissionService::sysDbPath() const
{
    return m_rootPath + "/sys";
}

DDL::Table PermissionService::permissionsTable() const
{
    DDL::Table table;
    table.name = "permissions";
    table.fields.append(DDL::Field("username", DDL::FieldType::VARCHAR, 64));
    table.fields.append(DDL::Field("database_name", DDL::FieldType::VARCHAR, 64));
    table.fields.append(DDL::Field("table_name", DDL::FieldType::VARCHAR, 64));
    table.fields.append(DDL::Field("action", DDL::FieldType::VARCHAR, 16));
    return table;
}

bool PermissionService::hasExplicitPermission(const QString& username,
                                              TableAction action,
                                              const QString& databaseName,
                                              const QString& tableName) const
{
    const QVector<QVector<QString>> permissions = loadPermissions();
    const QString actionText = actionToString(action);
    for (const auto& row : permissions) {
        if (row.size() < 4) {
            continue;
        }

        if (row[0] == username && row[1] == databaseName && row[2] == tableName && row[3] == actionText) {
            return true;
        }
    }

    return false;
}

QVector<QVector<QString>> PermissionService::loadPermissions() const
{
    return DDL::loadTableData(permissionsTable(), sysDbPath());
}

void PermissionService::savePermissions(const QVector<QVector<QString>>& permissions) const
{
    DDL::saveTableData(permissionsTable(), permissions, sysDbPath());
}

QString PermissionService::actionToString(TableAction action) const
{
    switch (action) {
    case TableAction::Select: return "SELECT";
    case TableAction::Insert: return "INSERT";
    case TableAction::Update: return "UPDATE";
    case TableAction::Delete: return "DELETE";
    case TableAction::Create: return "CREATE";
    case TableAction::Alter: return "ALTER";
    case TableAction::Drop: return "DROP";
    default: return "UNKNOWN";
    }
}

} // namespace DCL
