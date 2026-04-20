#ifndef SYSTEM_DB_BOOTSTRAP_H
#define SYSTEM_DB_BOOTSTRAP_H

#include <QString>

namespace DCL {

class SystemDbBootstrap {
public:
    explicit SystemDbBootstrap(const QString& rootPath = "../../dataDB");

    bool ensureInitialized(QString& error);
    QString systemDbPath() const;

private:
    bool ensureRegisteredInDbConfig(QString& error) const;

    QString m_rootPath;
};

} // namespace DCL

#endif // SYSTEM_DB_BOOTSTRAP_H
