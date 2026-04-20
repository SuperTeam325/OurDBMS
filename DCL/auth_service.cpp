#include "auth_service.h"

namespace DCL {

AuthService::AuthService(UserRepository& userRepository)
    : m_userRepository(userRepository) {}

bool AuthService::login(const QString& username, const QString& password, UserRecord& user, QString& error) const
{
    return m_userRepository.validateUser(username, password, user, error);
}

} // namespace DCL
