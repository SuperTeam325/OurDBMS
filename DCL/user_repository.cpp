#include "user_repository.h"
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRandomGenerator>

namespace DCL {

UserRepository::UserRepository(const QString& rootPath)
    : m_rootPath(rootPath) {}

bool UserRepository::ensureStorage(QString& error) const
{
    QDir dir;
    const QString sysPath = m_rootPath + "/sys";
    if (!dir.exists(sysPath) && !dir.mkpath(sysPath)) {
        error = "无法创建系统库目录";
        return false;
    }

    QFile file(usersFilePath());
    if (!file.exists()) {
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            error = "无法创建用户存储文件";
            return false;
        }
        file.write("[]");
        file.close();
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

    QFile file(usersFilePath());
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        error = "无法读取用户文件";
        return false;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    QJsonArray users = doc.isArray() ? doc.array() : QJsonArray();
    for (const QJsonValue& v : users) {
        const QJsonObject obj = v.toObject();
        if (obj.value("username").toString() == username) {
            error = "用户已存在";
            return false;
        }
    }

    const QString salt = generateSalt();
    const QString hash = hashPassword(plainPassword, salt);

    QJsonObject newUser;
    newUser.insert("username", username);
    newUser.insert("salt", salt);
    newUser.insert("hash", hash);
    newUser.insert("isAdmin", isAdmin);
    users.append(newUser);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        error = "无法写入用户文件";
        return false;
    }
    file.write(QJsonDocument(users).toJson(QJsonDocument::Indented));
    file.close();

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
    QFile file(usersFilePath());
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (!doc.isArray()) {
        return false;
    }

    for (const QJsonValue& v : doc.array()) {
        const QJsonObject obj = v.toObject();
        if (obj.value("username").toString() == username) {
            outUser.username = username;
            outUser.salt = obj.value("salt").toString();
            outUser.passwordHash = obj.value("hash").toString();
            outUser.isAdmin = obj.value("isAdmin").toBool(false);
            return true;
        }
    }

    return false;
}

QString UserRepository::usersFilePath() const
{
    return m_rootPath + "/sys/users.json";
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
