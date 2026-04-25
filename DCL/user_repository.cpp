#include "user_repository.h"
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QRandomGenerator>

namespace DCL {

UserRepository::UserRepository(const QString& rootPath)
    : m_rootPath(rootPath) {}

bool UserRepository::ensureStorage(QString& error) const
{
    QDir dir;
    const QString sysPath = sysDbPath();
    if (!dir.exists(sysPath) && !dir.mkpath(sysPath)) {
        error = "无法创建系统库目录";
        return false;
    }

    const QString dbsPath = sysPath + "/sys.dbs";
    QFile dbsFile(dbsPath);
    if (!dbsFile.exists()) {
        if (!dbsFile.open(QIODevice::WriteOnly)) {
            error = "无法创建系统库索引文件";
            return false;
        }
        dbsFile.close();
    }

    QString dbsPathRef = dbsPath;
    const QStringList tableNames = DDL::readFromDbs(dbsPathRef);
    if (!tableNames.contains("users")) {
        DDL::DataBase db;
        db.name = "sys";
        db.path = sysPath;
        DDL::Table table = usersTable();
        DDL::writeToDbs(db, table);
        DDL::saveSchema(table, db.path);
    }

    const QString usersDbfPath = sysPath + "/users/users.dbf";
    QFile usersDbf(usersDbfPath);
    if (!usersDbf.exists()) {
        DDL::saveTableData(usersTable(), {}, sysPath);
    }

    return true;
}

bool UserRepository::userExists(const QString& username) const
{
    UserRecord user;
    return getUser(username, user);
}

bool UserRepository::createUser(const QString& username, const QString& plainPassword, bool isAdmin, QString& error)
{
    if (username.trimmed().isEmpty()) {
        error = "用户名不能为空";
        return false;
    }

    if (plainPassword.isEmpty()) {
        error = "密码不能为空";
        return false;
    }

    if (!ensureStorage(error)) {
        return false;
    }

    QVector<QVector<QString>> users = DDL::loadTableData(usersTable(), sysDbPath());
    for (const auto& row : users) {
        if (row.size() >= 4 && row[0] == username) {
            error = "用户已存在";
            return false;
        }
    }

    const QString salt = generateSalt();
    const QString hash = hashPassword(plainPassword, salt);
    users.append({username, salt, hash, isAdmin ? "1" : "0"});
    DDL::saveTableData(usersTable(), users, sysDbPath());

    return true;
}

bool UserRepository::deleteUser(const QString& username, QString& error)
{
    const QString normalizedUsername = username.trimmed();
    if (normalizedUsername.isEmpty()) {
        error = "用户名不能为空";
        return false;
    }

    if (!ensureStorage(error)) {
        return false;
    }

    QVector<QVector<QString>> users = DDL::loadTableData(usersTable(), sysDbPath());
    QVector<QVector<QString>> filtered;
    bool removed = false;
    for (const auto& row : users) {
        if (row.size() >= 4 && row[0] == normalizedUsername) {
            removed = true;
            continue;
        }
        filtered.append(row);
    }

    if (!removed) {
        error = "用户不存在";
        return false;
    }

    DDL::saveTableData(usersTable(), filtered, sysDbPath());
    return true;
}

bool UserRepository::validateUser(const QString& username, const QString& plainPassword, UserRecord& outUser, QString& error) const
{
    if (!getUser(username, outUser)) {
        error = "用户名或密码错误";
        return false;
    }

    const QString computedHash = hashPassword(plainPassword, outUser.salt);
    if (computedHash != outUser.passwordHash) {
        error = "用户名或密码错误";
        return false;
    }

    return true;
}

bool UserRepository::getUser(const QString& username, UserRecord& outUser) const
{
    const QVector<QVector<QString>> users = DDL::loadTableData(usersTable(), sysDbPath());
    for (const auto& row : users) {
        if (row.size() >= 4 && row[0] == username) {
            outUser.username = username;
            outUser.salt = row[1];
            outUser.passwordHash = row[2];
            outUser.isAdmin = (row[3] == "1");
            return true;
        }
    }

    return false;
}

QString UserRepository::sysDbPath() const
{
    return m_rootPath + "/sys";
}

DDL::Table UserRepository::usersTable() const
{
    DDL::Table table;
    table.name = "users";
    table.fields.append(DDL::Field("username", DDL::FieldType::VARCHAR, 64));
    table.fields.append(DDL::Field("salt", DDL::FieldType::VARCHAR, 64));
    table.fields.append(DDL::Field("password_hash", DDL::FieldType::VARCHAR, 128));
    table.fields.append(DDL::Field("is_admin", DDL::FieldType::CHAR, 1));
    return table;
}

QString UserRepository::hashPassword(const QString& plainPassword, const QString& salt) const
{
    const QByteArray input = (salt + plainPassword).toUtf8();
    const QByteArray digest = QCryptographicHash::hash(input, QCryptographicHash::Sha256);
    return QString::fromUtf8(digest.toHex());
}

QString UserRepository::generateSalt() const
{
    QByteArray bytes;
    bytes.resize(16);
    for (int i = 0; i < bytes.size(); ++i) {
        bytes[i] = static_cast<char>(QRandomGenerator::global()->bounded(256));
    }
    return QString::fromUtf8(bytes.toHex());
}

} // namespace DCL
