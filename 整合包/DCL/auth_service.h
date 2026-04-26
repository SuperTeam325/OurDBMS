#ifndef AUTH_SERVICE_H
#define AUTH_SERVICE_H

#include "dcl_types.h"
#include "user_repository.h"

namespace DCL {

class AuthService {
public:
    explicit AuthService(UserRepository& userRepository);

    bool login(const QString& username, const QString& password, UserRecord& user, QString& error) const;

private:
    UserRepository& m_userRepository;
};

} // namespace DCL

#endif // AUTH_SERVICE_H
