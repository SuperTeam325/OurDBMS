#ifndef SESSION_MANAGER_H
#define SESSION_MANAGER_H

#include "dcl_types.h"

namespace DCL {

class SessionManager {
public:
    SessionManager();

    void setLoggedInUser(const UserRecord& user);
    void logout();
    void setCurrentDatabase(const QString& databaseName);
    const SessionContext& currentSession() const;

private:
    SessionContext m_session;
};

} // namespace DCL

#endif // SESSION_MANAGER_H
