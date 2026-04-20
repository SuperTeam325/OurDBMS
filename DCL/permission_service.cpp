#include "permission_service.h"
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace DCL {

PermissionService::PermissionService(const QString& rootPath)
    : m_rootPath(rootPath) {}

bool PermissionService::ensureStorage(QString& error) const
{
    QFile file(permissionsFilePath());
    if (!file.exists()) {
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            error = "无法创建权限文件";
            return false;
        }
        file.write("[]");
        file.close();
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

    // 普通用户默认当前数据库只读。
    if (action == TableAction::Select && !databaseName.isEmpty() && databaseName == session.currentDatabase) {
        return true;
    }

    if (hasExplicitPermission(session.username, action, databaseName, tableName)) {
        return true;
    }

    error = QString("权限不足：用户 %1 无法执行 %2").arg(session.username, actionToString(action));
    return false;
}

QString PermissionService::permissionsFilePath() const
{
    return m_rootPath + "/sys/permissions.json";
}

bool PermissionService::hasExplicitPermission(const QString& username,
                                              TableAction action,
                                              const QString& databaseName,
                                              const QString& tableName) const
{
    QFile file(permissionsFilePath());
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (!doc.isArray()) {
        return false;
    }

    const QString actionText = actionToString(action);
    for (const QJsonValue& value : doc.array()) {
        const QJsonObject obj = value.toObject();
        if (obj.value("username").toString() == username &&
            obj.value("database").toString() == databaseName &&
            obj.value("table").toString() == tableName &&
            obj.value("action").toString() == actionText) {
            return true;
        }
    }

    return false;
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
