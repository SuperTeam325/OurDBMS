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

    QString databaseName = m_sessionManager.currentSession().currentDatabase;
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

} // namespace DCL
