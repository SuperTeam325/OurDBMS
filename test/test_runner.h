#ifndef TEST_RUNNER_H
#define TEST_RUNNER_H

#include <QObject>
#include <QString>
#include <QStringList>

class MainWindow;

class TestRunner : public QObject
{
    Q_OBJECT
public:
    TestRunner(MainWindow *mainWindow,
               const QString &inputFile,
               const QString &outputFile,
               const QString &dbRootPath,
               QObject *parent = nullptr);

public slots:
    void start();

private slots:
    void processNext();

private:
    void setupDbPath();
    QStringList parseSqlFile(const QString &filePath);
    QString executeSqlAndCapture(const QString &sql);
    void writeResults();

    MainWindow   *m_mainWindow;
    QString       m_inputFile;
    QString       m_outputFile;
    QString       m_dbRootPath;
    QStringList   m_sqlStatements;
    QStringList   m_results;
    int           m_currentIndex = 0;
};

#endif // TEST_RUNNER_H
