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
    Create,
    Alter,
    Drop,
    Unknown
};

} // namespace DCL

#endif // DCL_TYPES_H
