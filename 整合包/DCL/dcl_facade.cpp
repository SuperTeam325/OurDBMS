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

    // sql语句与logout 语句匹配
    if (logoutMatch.hasMatch()) {
        if (!isLoggedIn()) {
            error = "当前没有已登录用户";
            return true;
        }
        logout();
        message = "已注销当前用户";
        return true;
    }

    // 尝试识别并处理登录相关语句：
    // 1) 如果匹配完整的 login 语法（LOGIN USER ... IDENTIFIED BY ...），使用完整匹配结果。
    // 2) 否则如果匹配简写 login 语法（LOGIN username password），使用简写匹配结果。
    // 3) 如果既不是 logout 也不是 login，则按顺序尝试其他会话相关的 DCL 语句处理器：
    //    CREATE USER / DROP USER / GRANT / REVOKE。每个处理器在匹配时会返回 true
    //    并通过 message/error 通知上层结果；返回 true 表示 SQL 已被 DCL 模块消费。
    QRegularExpressionMatch activeLoginMatch;
    if (loginFullMatch.hasMatch()) {
        // 匹配类似：LOGIN USER username IDENTIFIED BY 'password'
        activeLoginMatch = loginFullMatch;
    } else if (loginSimpleMatch.hasMatch()) {
        // 匹配简写形式：LOGIN username password
        activeLoginMatch = loginSimpleMatch;
    } else {
        // 不是 login 语句，尝试其它 DCL 会话/权限操作
        if (handleCreateUserSql(normalized, message, error)) {
            // CREATE USER 已被处理（成功或失败），返回 true 结束处理链
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
        // 未匹配任何 DCL 会话语句，返回 false 让调用方继续后续处理（如授权检查 + DDL）
        return false;
    }

    const QString username = activeLoginMatch.captured(1).trimmed();
    QString password = activeLoginMatch.captured(2).trimmed();
    if ((password.startsWith("'") && password.endsWith("'")) ||
        (password.startsWith("\"") && password.endsWith("\""))) {
        password = password.mid(1, password.size() - 2);
    }

    if (!login(username, password, error)) {
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

    const QString databaseName = m_sessionManager.currentSession().currentDatabase;
    return m_permissionService.checkPermission(m_sessionManager.currentSession(), action, databaseName, tableName, error);
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

    if (lower.startsWith("select")) {
        action = TableAction::Select;
        QRegularExpression re("from\\s+([a-zA-Z_][a-zA-Z0-9_]*)", QRegularExpression::CaseInsensitiveOption);
        QRegularExpressionMatch m = re.match(normalized);
        if (m.hasMatch()) tableName = m.captured(1);
    } else if (lower.startsWith("insert")) {
        action = TableAction::Insert;
        QRegularExpression re("into\\s+([a-zA-Z_][a-zA-Z0-9_]*)", QRegularExpression::CaseInsensitiveOption);
        QRegularExpressionMatch m = re.match(normalized);
        if (m.hasMatch()) tableName = m.captured(1);
    } else if (lower.startsWith("update")) {
        action = TableAction::Update;
        QRegularExpression re("update\\s+([a-zA-Z_][a-zA-Z0-9_]*)", QRegularExpression::CaseInsensitiveOption);
        QRegularExpressionMatch m = re.match(normalized);
        if (m.hasMatch()) tableName = m.captured(1);
    } else if (lower.startsWith("delete")) {
        action = TableAction::Delete;
        QRegularExpression re("from\\s+([a-zA-Z_][a-zA-Z0-9_]*)", QRegularExpression::CaseInsensitiveOption);
        QRegularExpressionMatch m = re.match(normalized);
        if (m.hasMatch()) tableName = m.captured(1);
    } else if (lower.startsWith("create table")) {
        action = TableAction::Create;
        QRegularExpression re("create\\s+table\\s+([a-zA-Z_][a-zA-Z0-9_]*)", QRegularExpression::CaseInsensitiveOption);
        QRegularExpressionMatch m = re.match(normalized);
        if (m.hasMatch()) tableName = m.captured(1);
    } else if (lower.startsWith("alter table")) {
        action = TableAction::Alter;
        QRegularExpression re("alter\\s+table\\s+([a-zA-Z_][a-zA-Z0-9_]*)", QRegularExpression::CaseInsensitiveOption);
        QRegularExpressionMatch m = re.match(normalized);
        if (m.hasMatch()) tableName = m.captured(1);
    } else if (lower.startsWith("drop table")) {
        action = TableAction::Drop;
        QRegularExpression re("drop\\s+table\\s+([a-zA-Z_][a-zA-Z0-9_]*)", QRegularExpression::CaseInsensitiveOption);
        QRegularExpressionMatch m = re.match(normalized);
        if (m.hasMatch()) tableName = m.captured(1);
    }

    return true;
}

TableAction DclFacade::parseActionFromText(const QString& text) const
{
    const QString upper = text.trimmed().toUpper();
    if (upper == "SELECT") return TableAction::Select;
    if (upper == "INSERT") return TableAction::Insert;
    if (upper == "UPDATE") return TableAction::Update;
    if (upper == "DELETE") return TableAction::Delete;
    if (upper == "CREATE") return TableAction::Create;
    if (upper == "ALTER") return TableAction::Alter;
    if (upper == "DROP") return TableAction::Drop;
    return TableAction::Unknown;
}

bool DclFacade::handleGrantSql(const QString& sql, QString& message, QString& error)
{
    QRegularExpression re(
        "^grant\\s+(.+)\\s+on\\s+([a-zA-Z_][a-zA-Z0-9_]*)\\s+to\\s+['\"]?([a-zA-Z_][a-zA-Z0-9_]*)['\"]?\\s*;?$",
        QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch m = re.match(sql);
    if (!m.hasMatch()) {
        return false;
    }

    if (!isLoggedIn() || !currentSession().isAdmin) {
        error = "只有管理员可以执行 GRANT";
        return true;
    }

    const QString actionList = m.captured(1);
    const QString tableName = m.captured(2);
    const QString username = m.captured(3);
    const QString databaseName = currentSession().currentDatabase;

    if (databaseName.isEmpty()) {
        error = "请先 USE 数据库后再执行 GRANT";
        return true;
    }

    UserRecord targetUser;
    if (!m_userRepository.getUser(username, targetUser)) {
        error = "授权失败：目标用户不存在";
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
        "^revoke\\s+(.+)\\s+on\\s+([a-zA-Z_][a-zA-Z0-9_]*)\\s+from\\s+['\"]?([a-zA-Z_][a-zA-Z0-9_]*)['\"]?\\s*;?$",
        QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch m = re.match(sql);
    if (!m.hasMatch()) {
        return false;
    }

    if (!isLoggedIn() || !currentSession().isAdmin) {
        error = "只有管理员可以执行 REVOKE";
        return true;
    }

    const QString actionList = m.captured(1);
    const QString tableName = m.captured(2);
    const QString username = m.captured(3);
    const QString databaseName = currentSession().currentDatabase;

    if (databaseName.isEmpty()) {
        error = "请先 USE 数据库后再执行 REVOKE";
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

    if (!isLoggedIn() || !currentSession().isAdmin) {
        error = "只有管理员可以执行 CREATE USER";
        return true;
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

    if (!isLoggedIn() || !currentSession().isAdmin) {
        error = "只有管理员可以执行 DROP USER";
        return true;
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
