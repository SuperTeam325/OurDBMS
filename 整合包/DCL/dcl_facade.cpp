#include "dcl_facade.h"
#include <QRegularExpression>

namespace DCL {

DclFacade::DclFacade(const QString& rootPath)
    : m_bootstrap(rootPath)
    , m_userRepository(rootPath)
    , m_authService(m_userRepository)
    , m_sessionManager()
    , m_permissionService(rootPath)
{}

bool DclFacade::initialize(QString& error)
{
    if (!m_bootstrap.ensureInitialized(error)) {
        return false;
    }

    if (!m_permissionService.ensureStorage(error)) {
        return false;
    }

    return true;
}

bool DclFacade::login(const QString& username, const QString& password, QString& error)
{
    UserRecord user;
    if (!m_authService.login(username.trimmed(), password, user, error)) {
        return false;
    }

    m_sessionManager.setLoggedInUser(user);
    return true;
}

void DclFacade::logout()
{
    m_sessionManager.logout();
}

void DclFacade::setCurrentDatabase(const QString& databaseName)
{
    m_sessionManager.setCurrentDatabase(databaseName);
}

bool DclFacade::isLoggedIn() const
{
    return m_sessionManager.currentSession().isLoggedIn;
}

const SessionContext& DclFacade::currentSession() const
{
    return m_sessionManager.currentSession();
}

bool DclFacade::tryHandleSessionSql(const QString& sql, QString& message, QString& error)
{
    const QString normalized = sql.trimmed();
    if (normalized.isEmpty()) {
        return false;
    }

    QRegularExpression loginSimpleRe(
        "^login\\s+([a-zA-Z_][a-zA-Z0-9_]*)\\s+(.+?)\\s*;?$",
        QRegularExpression::CaseInsensitiveOption);
    QRegularExpression loginFullRe(
        "^login\\s+user\\s+([a-zA-Z_][a-zA-Z0-9_]*)\\s+identified\\s+by\\s+(.+?)\\s*;?$",
        QRegularExpression::CaseInsensitiveOption);
    QRegularExpression logoutRe(
        "^logout\\s*;?$",
        QRegularExpression::CaseInsensitiveOption);

    QRegularExpressionMatch loginSimpleMatch = loginSimpleRe.match(normalized);
    QRegularExpressionMatch loginFullMatch = loginFullRe.match(normalized);
    QRegularExpressionMatch logoutMatch = logoutRe.match(normalized);

    if (logoutMatch.hasMatch()) {
        if (!isLoggedIn()) {
            error = "当前没有已登录用户";
            return true;
        }
        logout();
        message = "已注销当前用户";
        return true;
    }

    QRegularExpressionMatch activeLoginMatch;
    if (loginFullMatch.hasMatch()) {
        activeLoginMatch = loginFullMatch;
    } else if (loginSimpleMatch.hasMatch()) {
        activeLoginMatch = loginSimpleMatch;
    } else {
        if (handleCreateUserSql(normalized, message, error)) {
            return true;
        }
        if (handleDropUserSql(normalized, message, error)) {
            return true;
        }
        if (handleGrantSql(normalized, message, error)) {
            return true;
        }
        if (handleRevokeSql(normalized, message, error)) {
            return true;
        }
        return false;
    }

    const QString username = activeLoginMatch.captured(1).trimmed();
    QString password = activeLoginMatch.captured(2).trimmed();
    if ((password.startsWith("'") && password.endsWith("'")) ||
        (password.startsWith("\"") && password.endsWith("\""))) {
        password = password.mid(1, password.size() - 2);
    }

    if (!login(username.trimmed().toLower(), password, error)) {
        return true;
    }

    message = "用户切换成功：" + currentSession().username;
    return true;
}

bool DclFacade::authorizeSql(const QString& sql, QString& error) const
{
    TableAction action = TableAction::Unknown;
    QString tableName;
    if (!parseSqlActionAndTable(sql, action, tableName)) {
        error = "SQL 语句为空";
        return false;
    }

    if (action == TableAction::Unknown) {
        if (!m_sessionManager.currentSession().isLoggedIn) {
            error = "未登录，禁止执行 SQL";
            return false;
        }
        return true;
    }

    QString checkDb;
    if (action == TableAction::CreateDatabase || action == TableAction::DropDatabase ||
        action == TableAction::CreateUser || action == TableAction::DropUser ||
        action == TableAction::GrantPrivilege || action == TableAction::RevokePrivilege) {
        checkDb = "*";
    } else {
        checkDb = m_sessionManager.currentSession().currentDatabase;
        if (checkDb.isEmpty() && action != TableAction::Select) {
            error = "未指定当前数据库，请先执行 USE <数据库名>";
            return false;
        }
    }

    return m_permissionService.checkPermission(m_sessionManager.currentSession(), action, checkDb, tableName, error);
}

bool DclFacade::parseSqlActionAndTable(const QString& sql, TableAction& action, QString& tableName) const
{
    const QString normalized = sql.trimmed();
    if (normalized.isEmpty()) {
        return false;
    }

    const QString lower = normalized.toLower();
    action = TableAction::Unknown;
    tableName.clear();

    if (lower.startsWith("create database")) {
        action = TableAction::CreateDatabase;
        QRegularExpression re("create\\s+database\\s+([a-zA-Z_][a-zA-Z0-9_]*)",
                              QRegularExpression::CaseInsensitiveOption);
        QRegularExpressionMatch m = re.match(normalized);
        if (m.hasMatch()) tableName = m.captured(1);
    } else if (lower.startsWith("drop database")) {
        action = TableAction::DropDatabase;
        QRegularExpression re("drop\\s+database\\s+([a-zA-Z_][a-zA-Z0-9_]*)",
                              QRegularExpression::CaseInsensitiveOption);
        QRegularExpressionMatch m = re.match(normalized);
        if (m.hasMatch()) tableName = m.captured(1);
    } else if (lower.startsWith("create user")) {
        action = TableAction::CreateUser;
        QRegularExpression re("create\\s+user\\s+([a-zA-Z_][a-zA-Z0-9_]*)",
                              QRegularExpression::CaseInsensitiveOption);
        QRegularExpressionMatch m = re.match(normalized);
        if (m.hasMatch()) tableName = m.captured(1);
    } else if (lower.startsWith("drop user")) {
        action = TableAction::DropUser;
        QRegularExpression re("drop\\s+user\\s+['\"]?([a-zA-Z_][a-zA-Z0-9_]*)['\"]?",
                              QRegularExpression::CaseInsensitiveOption);
        QRegularExpressionMatch m = re.match(normalized);
        if (m.hasMatch()) tableName = m.captured(1);
    } else if (lower.startsWith("grant")) {
        action = TableAction::GrantPrivilege;
    } else if (lower.startsWith("revoke")) {
        action = TableAction::RevokePrivilege;
    } else if (lower.startsWith("create table")) {
        action = TableAction::CreateTable;
        QRegularExpression re("create\\s+table\\s+([a-zA-Z_][a-zA-Z0-9_]*)", QRegularExpression::CaseInsensitiveOption);
        QRegularExpressionMatch m = re.match(normalized);
        if (m.hasMatch()) tableName = m.captured(1);
    } else if (lower.startsWith("alter table")) {
        action = TableAction::Alter;
        QRegularExpression re("alter\\s+table\\s+([a-zA-Z_][a-zA-Z0-9_]*)", QRegularExpression::CaseInsensitiveOption);
        QRegularExpressionMatch m = re.match(normalized);
        if (m.hasMatch()) tableName = m.captured(1);
    } else if (lower.startsWith("drop table")) {
        action = TableAction::DropTable;
        QRegularExpression re("drop\\s+table\\s+([a-zA-Z_][a-zA-Z0-9_]*)", QRegularExpression::CaseInsensitiveOption);
        QRegularExpressionMatch m = re.match(normalized);
        if (m.hasMatch()) tableName = m.captured(1);
    } else if (lower.startsWith("select")) {
        action = TableAction::Select;
        QRegularExpression re("select\\s+.+\\s+from\\s+([a-zA-Z_][a-zA-Z0-9_]*)", QRegularExpression::CaseInsensitiveOption);
        QRegularExpressionMatch m = re.match(normalized);
        if (m.hasMatch()) tableName = m.captured(1);
    } else if (lower.startsWith("insert")) {
        action = TableAction::Insert;
        QRegularExpression re("insert\\s+into\\s+([a-zA-Z_][a-zA-Z0-9_]*)", QRegularExpression::CaseInsensitiveOption);
        QRegularExpressionMatch m = re.match(normalized);
        if (m.hasMatch()) tableName = m.captured(1);
    } else if (lower.startsWith("update")) {
        action = TableAction::Update;
        QRegularExpression re("update\\s+([a-zA-Z_][a-zA-Z0-9_]*)", QRegularExpression::CaseInsensitiveOption);
        QRegularExpressionMatch m = re.match(normalized);
        if (m.hasMatch()) tableName = m.captured(1);
    } else if (lower.startsWith("delete")) {
        action = TableAction::Delete;
        QRegularExpression re("delete\\s+from\\s+([a-zA-Z_][a-zA-Z0-9_]*)", QRegularExpression::CaseInsensitiveOption);
        QRegularExpressionMatch m = re.match(normalized);
        if (m.hasMatch()) tableName = m.captured(1);
    }

    return true;
}

TableAction DclFacade::parseActionFromText(const QString& text) const
{
    const QString upper = text.trimmed().toUpper();
    if (upper == "SELECT")          return TableAction::Select;
    if (upper == "INSERT")          return TableAction::Insert;
    if (upper == "UPDATE")          return TableAction::Update;
    if (upper == "DELETE")          return TableAction::Delete;
    if (upper == "CREATE DATABASE") return TableAction::CreateDatabase;
    if (upper == "DROP DATABASE")   return TableAction::DropDatabase;
    if (upper == "CREATE TABLE")    return TableAction::CreateTable;
    if (upper == "CREATE")          return TableAction::CreateTable;  // legacy migration
    if (upper == "DROP TABLE")      return TableAction::DropTable;
    if (upper == "DROP")            return TableAction::DropTable;    // legacy migration
    if (upper == "ALTER")           return TableAction::Alter;
    if (upper == "CREATE USER")     return TableAction::CreateUser;
    if (upper == "DROP USER")       return TableAction::DropUser;
    if (upper == "GRANT")           return TableAction::GrantPrivilege;
    if (upper == "REVOKE")          return TableAction::RevokePrivilege;
    if (upper == "ALL PRIVILEGES")  return TableAction::AllPrivileges;
    return TableAction::Unknown;
}

bool DclFacade::handleGrantSql(const QString& sql, QString& message, QString& error)
{
    QRegularExpression re(
        "^grant\\s+(.+)\\s+on\\s+([a-zA-Z_*][\\w*]*)\\s*\\.\\s*([a-zA-Z_*][\\w*]*)\\s+to\\s+['\"]?([a-zA-Z_][a-zA-Z0-9_]*)['\"]?\\s*;?$",
        QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch m = re.match(sql);
    if (!m.hasMatch()) {
        if (sql.trimmed().toLower().startsWith("grant")) {
            error = "GRANT 语法错误，支持的格式: GRANT <action> ON <db>.<table> TO <user>;";
            return true;
        }
        return false;
    }

    if (!isLoggedIn()) {
        error = "未登录";
        return true;
    }

    QString permError;
    if (!m_permissionService.checkPermission(currentSession(), TableAction::GrantPrivilege, "*", "*", permError)) {
        if (!currentSession().isAdmin) {
            error = "权限不足：需要 GRANT 权限";
            return true;
        }
    }

    const QString actionList = m.captured(1).trimmed();
    const QString databaseName = m.captured(2);
    const QString tableName = m.captured(3);
    const QString username = m.captured(4);

    UserRecord targetUser;
    if (!m_userRepository.getUser(username, targetUser)) {
        error = "授权失败：目标用户不存在";
        return true;
    }

    if (actionList.toUpper() == "ALL PRIVILEGES") {
        static const TableAction allActions[] = {
            TableAction::Select, TableAction::Insert, TableAction::Update,
            TableAction::Delete, TableAction::CreateDatabase, TableAction::DropDatabase,
            TableAction::CreateTable, TableAction::DropTable, TableAction::Alter,
            TableAction::CreateUser, TableAction::DropUser,
            TableAction::GrantPrivilege, TableAction::RevokePrivilege
        };

        QString serviceError;
        for (const auto& a : allActions) {
            if (!m_permissionService.grantPermission(currentSession(), username, a, databaseName, tableName, serviceError)) {
                error = serviceError;
                return true;
            }
        }

        if (!m_permissionService.grantPermission(currentSession(), username, TableAction::AllPrivileges, databaseName, tableName, serviceError)) {
            error = serviceError;
            return true;
        }

        if (databaseName == "*" && tableName == "*") {
            if (!m_userRepository.setUserAdmin(username, true, serviceError)) {
                error = serviceError;
                return true;
            }
        }

        message = "授权成功：ALL PRIVILEGES";
        return true;
    }

    const QStringList actions = actionList.split(',', Qt::SkipEmptyParts);
    int grantedCount = 0;
    for (const QString& actionText : actions) {
        const TableAction action = parseActionFromText(actionText);
        if (action == TableAction::Unknown) {
            error = "GRANT 失败：存在不支持的权限动作";
            return true;
        }

        QString serviceError;
        if (!m_permissionService.grantPermission(currentSession(), username, action, databaseName, tableName, serviceError)) {
            error = serviceError;
            return true;
        }
        grantedCount++;
    }

    message = QString("授权成功：%1 条权限").arg(grantedCount);
    return true;
}

bool DclFacade::handleRevokeSql(const QString& sql, QString& message, QString& error)
{
    QRegularExpression re(
        "^revoke\\s+(.+)\\s+on\\s+([a-zA-Z_*][\\w*]*)\\s*\\.\\s*([a-zA-Z_*][\\w*]*)\\s+from\\s+['\"]?([a-zA-Z_][a-zA-Z0-9_]*)['\"]?\\s*;?$",
        QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch m = re.match(sql);
    if (!m.hasMatch()) {
        if (sql.trimmed().toLower().startsWith("revoke")) {
            error = "REVOKE 语法错误，支持的格式: REVOKE <action> ON <db>.<table> FROM <user>;";
            return true;
        }
        return false;
    }

    if (!isLoggedIn()) {
        error = "未登录";
        return true;
    }

    QString permError;
    if (!m_permissionService.checkPermission(currentSession(), TableAction::RevokePrivilege, "*", "*", permError)) {
        if (!currentSession().isAdmin) {
            error = "权限不足：需要 REVOKE 权限";
            return true;
        }
    }

    const QString actionList = m.captured(1).trimmed();
    const QString databaseName = m.captured(2);
    const QString tableName = m.captured(3);
    const QString username = m.captured(4);

    if (actionList.toUpper() == "ALL PRIVILEGES") {
        static const TableAction allActions[] = {
            TableAction::Select, TableAction::Insert, TableAction::Update,
            TableAction::Delete, TableAction::CreateDatabase, TableAction::DropDatabase,
            TableAction::CreateTable, TableAction::DropTable, TableAction::Alter,
            TableAction::CreateUser, TableAction::DropUser,
            TableAction::GrantPrivilege, TableAction::RevokePrivilege
        };

        QString serviceError;
        for (const auto& a : allActions) {
            m_permissionService.revokePermission(currentSession(), username, a, databaseName, tableName, serviceError);
        }
        m_permissionService.revokePermission(currentSession(), username, TableAction::AllPrivileges, databaseName, tableName, serviceError);

        if (databaseName == "*" && tableName == "*") {
            if (!m_userRepository.setUserAdmin(username, false, serviceError)) {
                error = serviceError;
                return true;
            }
        }

        message = "撤权成功：ALL PRIVILEGES";
        return true;
    }

    const QStringList actions = actionList.split(',', Qt::SkipEmptyParts);
    int revokedCount = 0;
    for (const QString& actionText : actions) {
        const TableAction action = parseActionFromText(actionText);
        if (action == TableAction::Unknown) {
            error = "REVOKE 失败：存在不支持的权限动作";
            return true;
        }

        QString serviceError;
        if (!m_permissionService.revokePermission(currentSession(), username, action, databaseName, tableName, serviceError)) {
            error = serviceError;
            return true;
        }
        revokedCount++;
    }

    message = QString("撤权成功：%1 条权限").arg(revokedCount);
    return true;
}

bool DclFacade::handleCreateUserSql(const QString& sql, QString& message, QString& error)
{
    QRegularExpression re(
        "^create\\s+user\\s+([a-zA-Z_][a-zA-Z0-9_]*)\\s+identified\\s+by\\s+(.+?)\\s*;?$",
        QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch m = re.match(sql);
    if (!m.hasMatch()) {
        return false;
    }

    if (!isLoggedIn()) {
        error = "未登录";
        return true;
    }

    QString permError;
    if (!m_permissionService.checkPermission(currentSession(), TableAction::CreateUser, "*", "*", permError)) {
        if (!currentSession().isAdmin) {
            error = "权限不足：需要 CREATE USER 权限";
            return true;
        }
    }

    const QString username = m.captured(1).trimmed();
    QString password = m.captured(2).trimmed();
    if ((password.startsWith("'") && password.endsWith("'")) ||
        (password.startsWith("\"") && password.endsWith("\""))) {
        password = password.mid(1, password.size() - 2);
    }

    if (!m_userRepository.createUser(username, password, false, error)) {
        return true;
    }

    message = "用户创建成功：" + username;
    return true;
}

bool DclFacade::handleDropUserSql(const QString& sql, QString& message, QString& error)
{
    QRegularExpression re(
        "^drop\\s+user\\s+['\"]?([a-zA-Z_][a-zA-Z0-9_]*)['\"]?\\s*;?$",
        QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch m = re.match(sql);
    if (!m.hasMatch()) {
        return false;
    }

    if (!isLoggedIn()) {
        error = "未登录";
        return true;
    }

    QString permError;
    if (!m_permissionService.checkPermission(currentSession(), TableAction::DropUser, "*", "*", permError)) {
        if (!currentSession().isAdmin) {
            error = "权限不足：需要 DROP USER 权限";
            return true;
        }
    }

    const QString username = m.captured(1).trimmed();
    UserRecord targetUser;
    if (!m_userRepository.getUser(username, targetUser)) {
        error = "删除失败：目标用户不存在";
        return true;
    }

    if (targetUser.isAdmin) {
        error = "删除失败：不允许删除管理员用户";
        return true;
    }

    if (currentSession().username == username) {
        error = "删除失败：不允许删除当前登录用户";
        return true;
    }

    if (!m_userRepository.deleteUser(username, error)) {
        return true;
    }

    QString permissionError;
    if (!m_permissionService.removePermissionsForUser(username, permissionError)) {
        error = permissionError;
        return true;
    }

    message = "用户删除成功：" + username;
    return true;
}

} // namespace DCL
