#ifndef DCL_TYPES_H
#define DCL_TYPES_H

#include <QString>

namespace DCL {

struct UserRecord {
    QString username;
    QString passwordHash;
    QString salt;
    bool isAdmin = false;
};

struct SessionContext {
    bool isLoggedIn = false;
    QString username;
    bool isAdmin = false;
    QString currentDatabase;
};

enum class TableAction {
    Select,
    Insert,
    Update,
    Delete,
    CreateDatabase,
    DropDatabase,
    CreateTable,    // was Create
    DropTable,      // was Drop
    Alter,          // all ALTER TABLE sub-types
    CreateUser,
    DropUser,
    GrantPrivilege,
    RevokePrivilege,
    AllPrivileges,  // sentinel for ALL PRIVILEGES
    Unknown
};

} // namespace DCL

#endif // DCL_TYPES_H
