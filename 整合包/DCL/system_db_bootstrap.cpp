#include "system_db_bootstrap.h"
#include "permission_service.h"
#include "user_repository.h"
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

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

    if (!ensureRegisteredInDbConfig(error)) {
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

bool SystemDbBootstrap::ensureRegisteredInDbConfig(QString& error) const
{
    QFile file("db_config.json");
    QJsonObject obj;

    if (file.exists()) {
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            error = "无法读取 db_config.json";
            return false;
        }

        const QByteArray data = file.readAll();
        file.close();

        if (!data.trimmed().isEmpty()) {
            const QJsonDocument doc = QJsonDocument::fromJson(data);
            if (doc.isNull() || !doc.isObject()) {
                error = "db_config.json 格式无效";
                return false;
            }
            obj = doc.object();
        }
    }

    const QString sysPath = systemDbPath();
    if (obj.contains("sys") && obj.value("sys").toString() == sysPath) {
        return true;
    }

    obj["sys"] = sysPath;
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        error = "无法写入 db_config.json";
        return false;
    }

    file.write(QJsonDocument(obj).toJson());
    file.close();
    return true;
}

} // namespace DCL
