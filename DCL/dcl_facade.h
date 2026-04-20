#ifndef DCL_FACADE_H
#define DCL_FACADE_H

#include "auth_service.h"
#include "permission_service.h"
#include "session_manager.h"
#include "system_db_bootstrap.h"
#include "user_repository.h"
#include "dcl_types.h"
#include <QString>

namespace DCL {

class DclFacade {
public:
    explicit DclFacade(const QString& rootPath = "../../dataDB");

    bool initialize(QString& error);
    bool login(const QString& username, const QString& password, QString& error);
    void logout();
    void setCurrentDatabase(const QString& databaseName);

    bool isLoggedIn() const;
    const SessionContext& currentSession() const;

    bool tryHandleSessionSql(const QString& sql, QString& message, QString& error);
    bool authorizeSql(const QString& sql, QString& error) const;

private:
    bool parseSqlActionAndTable(const QString& sql, TableAction& action, QString& tableName) const;

    SystemDbBootstrap m_bootstrap;
    UserRepository m_userRepository;
    AuthService m_authService;
    SessionManager m_sessionManager;
    PermissionService m_permissionService;
};

} // namespace DCL

#endif // DCL_FACADE_H
