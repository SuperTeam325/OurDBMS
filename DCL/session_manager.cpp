#include "session_manager.h"

namespace DCL {

SessionManager::SessionManager() = default;

void SessionManager::setLoggedInUser(const UserRecord& user)
{
    m_session.isLoggedIn = true;
    m_session.username = user.username;
    m_session.isAdmin = user.isAdmin;
}

void SessionManager::logout()
{
    m_session = SessionContext{};
}

void SessionManager::setCurrentDatabase(const QString& databaseName)
{
    m_session.currentDatabase = databaseName;
}

const SessionContext& SessionManager::currentSession() const
{
    return m_session;
}

} // namespace DCL
