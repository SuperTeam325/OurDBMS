#include "system_db_bootstrap.h"
#include "permission_service.h"
#include "user_repository.h"
#include <QDir>

namespace DCL {

SystemDbBootstrap::SystemDbBootstrap(const QString& rootPath)
    : m_rootPath(rootPath) {}

bool SystemDbBootstrap::ensureInitialized(QString& error)
{
    QDir dir;
    if (!dir.exists(m_rootPath) && !dir.mkpath(m_rootPath)) {
        error = "无法创建数据根目录";
        return false;
    }

    const QString sysPath = systemDbPath();
    if (!dir.exists(sysPath) && !dir.mkpath(sysPath)) {
        error = "无法创建系统数据库目录";
        return false;
    }

    UserRepository users(m_rootPath);
    if (!users.ensureStorage(error)) {
        return false;
    }

    PermissionService permissions(m_rootPath);
    if (!permissions.ensureStorage(error)) {
        return false;
    }

    if (!users.userExists("admin")) {
        if (!users.createUser("admin", "123456", true, error)) {
            return false;
        }
    }

    return true;
}

QString SystemDbBootstrap::systemDbPath() const
{
    return m_rootPath + "/sys";
}

} // namespace DCL
