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

    const QString permissionsDbfPath = sysPath + "/permissions/permissions.tbf";
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

    if (hasAllPrivileges(session.username)) {
        return true;
    }

    if (action == TableAction::Select) {
        return true;
    }

    if (action == TableAction::Unknown) {
        error = "不支持的 SQL 操作";
        return false;
    }

    if (hasHierarchicalPermission(session.username, action, databaseName, tableName)) {
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

    const QString normUsername = normalizeIdentifier(username);
    const QString normDatabase = normalizeIdentifier(databaseName);
    const QString normTable = normalizeIdentifier(tableName);
    const QString actionText = actionToString(action);

    QVector<QVector<QString>> permissions = loadPermissions();
    for (const auto& row : permissions) {
        if (row.size() >= 4
            && normalizeIdentifier(row[0]) == normUsername
            && normalizeIdentifier(row[1]) == normDatabase
            && normalizeIdentifier(row[2]) == normTable
            && row[3] == actionText) {
            return true;
        }
    }

    permissions.append({normUsername, normDatabase, normTable, actionText});
    if (!savePermissions(permissions)) {
        error = "授权失败：无法写入权限文件";
        return false;
    }
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

    const QString normUsername = normalizeIdentifier(username);
    const QString normDatabase = normalizeIdentifier(databaseName);
    const QString normTable = normalizeIdentifier(tableName);
    const QString actionText = actionToString(action);

    QVector<QVector<QString>> permissions = loadPermissions();
    QVector<QVector<QString>> filtered;
    bool removed = false;
    for (const auto& row : permissions) {
        if (row.size() >= 4
            && normalizeIdentifier(row[0]) == normUsername
            && normalizeIdentifier(row[1]) == normDatabase
            && normalizeIdentifier(row[2]) == normTable
            && row[3] == actionText) {
            removed = true;
            continue;
        }
        filtered.append(row);
    }

    if (!removed) {
        return true;
    }

    if (!savePermissions(filtered)) {
        error = "撤权失败：无法写入权限文件";
        return false;
    }
    return true;
}

bool PermissionService::removePermissionsForUser(const QString& username, QString& error)
{
    const QString normUsername = normalizeIdentifier(username);
    if (normUsername.isEmpty()) {
        error = "用户名不能为空";
        return false;
    }

    QVector<QVector<QString>> permissions = loadPermissions();
    QVector<QVector<QString>> filtered;
    for (const auto& row : permissions) {
        if (row.size() >= 4 && normalizeIdentifier(row[0]) == normUsername) {
            continue;
        }
        filtered.append(row);
    }

    if (!savePermissions(filtered)) {
        error = "删除用户权限失败：无法写入权限文件";
        return false;
    }
    return true;
}

bool PermissionService::migrateActionNames(QString& error)
{
    QVector<QVector<QString>> permissions = loadPermissions();
    bool changed = false;
    for (auto& row : permissions) {
        if (row.size() >= 4) {
            if (row[3] == "CREATE") {
                row[3] = "CREATE TABLE";
                changed = true;
            } else if (row[3] == "DROP") {
                row[3] = "DROP TABLE";
                changed = true;
            }
        }
    }
    if (changed) {
        if (!savePermissions(permissions)) {
            error = "权限迁移失败：无法写入权限文件";
            return false;
        }
    }
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
    table.fields.append(DDL::Field("action", DDL::FieldType::VARCHAR, 32));
    return table;
}

bool PermissionService::hasExplicitPermission(const QString& username,
                                              TableAction action,
                                              const QString& databaseName,
                                              const QString& tableName) const
{
    const QVector<QVector<QString>> permissions = loadPermissions();
    const QString actionText = actionToString(action);
    const QString normUsername = normalizeIdentifier(username);
    const QString normDatabase = normalizeIdentifier(databaseName);
    const QString normTable = normalizeIdentifier(tableName);
    for (const auto& row : permissions) {
        if (row.size() < 4) {
            continue;
        }

        if (normalizeIdentifier(row[0]) == normUsername
            && normalizeIdentifier(row[1]) == normDatabase
            && normalizeIdentifier(row[2]) == normTable
            && row[3] == actionText) {
            return true;
        }
    }

    return false;
}

bool PermissionService::hasAllPrivileges(const QString& username) const
{
    const QVector<QVector<QString>> permissions = loadPermissions();
    const QString normUsername = normalizeIdentifier(username);
    for (const auto& row : permissions) {
        if (row.size() >= 4 &&
            normalizeIdentifier(row[0]) == normUsername &&
            row[1] == "*" &&
            row[2] == "*" &&
            row[3] == "ALL PRIVILEGES") {
            return true;
        }
    }
    return false;
}

bool PermissionService::hasHierarchicalPermission(const QString& username,
                                                  TableAction action,
                                                  const QString& databaseName,
                                                  const QString& tableName) const
{
    const QVector<QVector<QString>> permissions = loadPermissions();
    const QString actionText = actionToString(action);
    const QString normUsername = normalizeIdentifier(username);
    const QString normDatabase = normalizeIdentifier(databaseName);
    const QString normTable = normalizeIdentifier(tableName);

    for (const auto& row : permissions) {
        if (row.size() < 4) continue;
        if (normalizeIdentifier(row[0]) != normUsername) continue;
        if (row[3] != actionText && row[3] != "ALL PRIVILEGES") continue;

        if (row[1] == "*" && row[2] == "*") return true;

        if (row[2] == "*" && (normDatabase.isEmpty() || normalizeIdentifier(row[1]) == normDatabase))
            return true;

        if (!normDatabase.isEmpty() && !normTable.isEmpty() &&
            normalizeIdentifier(row[1]) == normDatabase && normalizeIdentifier(row[2]) == normTable)
            return true;
    }
    return false;
}

QVector<QVector<QString>> PermissionService::loadPermissions() const
{
    return DDL::loadTableData(permissionsTable(), sysDbPath());
}

bool PermissionService::savePermissions(const QVector<QVector<QString>>& permissions) const
{
    return DDL::saveTableData(permissionsTable(), permissions, sysDbPath());
}

QString PermissionService::actionToString(TableAction action) const
{
    switch (action) {
    case TableAction::Select:          return "SELECT";
    case TableAction::Insert:          return "INSERT";
    case TableAction::Update:          return "UPDATE";
    case TableAction::Delete:          return "DELETE";
    case TableAction::CreateDatabase:  return "CREATE DATABASE";
    case TableAction::DropDatabase:    return "DROP DATABASE";
    case TableAction::CreateTable:     return "CREATE TABLE";
    case TableAction::DropTable:       return "DROP TABLE";
    case TableAction::Alter:           return "ALTER";
    case TableAction::CreateUser:      return "CREATE USER";
    case TableAction::DropUser:        return "DROP USER";
    case TableAction::GrantPrivilege:  return "GRANT";
    case TableAction::RevokePrivilege: return "REVOKE";
    case TableAction::AllPrivileges:   return "ALL PRIVILEGES";
    default: return "UNKNOWN";
    }
}

QString PermissionService::normalizeIdentifier(const QString& s)
{
    return s.trimmed().toLower();
}

} // namespace DCL
