#ifndef USER_REPOSITORY_H
#define USER_REPOSITORY_H

#include "dcl_types.h"
#include <QString>

namespace DCL {

class UserRepository {
public:
    explicit UserRepository(const QString& rootPath = "../../dataDB");

    bool ensureStorage(QString& error) const;
    bool userExists(const QString& username) const;
    bool createUser(const QString& username, const QString& plainPassword, bool isAdmin, QString& error);
    bool validateUser(const QString& username, const QString& plainPassword, UserRecord& outUser, QString& error) const;
    bool getUser(const QString& username, UserRecord& outUser) const;

private:
    QString usersFilePath() const;
    QString hashPassword(const QString& plainPassword, const QString& salt) const;
    QString generateSalt() const;

    QString m_rootPath;
};

} // namespace DCL

#endif // USER_REPOSITORY_H
