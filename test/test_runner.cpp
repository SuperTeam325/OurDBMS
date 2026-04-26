#include "test_runner.h"
#include "../DDL/mainwindow.h"
#include "../DDL/dialog.h"

#include <QApplication>
#include <QDateTime>
#include <QFile>
#include <QLineEdit>
#include <QMetaObject>
#include <QPushButton>
#include <QTextEdit>
#include <QTextStream>
#include <QTimer>

TestRunner::TestRunner(MainWindow *mainWindow,
                       const QString &inputFile,
                       const QString &outputFile,
                       const QString &dbRootPath,
                       QObject *parent)
    : QObject(parent)
    , m_mainWindow(mainWindow)
    , m_inputFile(inputFile)
    , m_outputFile(outputFile)
    , m_dbRootPath(dbRootPath)
{
}

void TestRunner::start()
{
    m_sqlStatements = parseSqlFile(m_inputFile);
    if (m_sqlStatements.isEmpty()) {
        QTextStream(stderr) << "Error: No SQL statements found in " << m_inputFile << "\n";
        QApplication::quit();
        return;
    }

    QTextStream(stdout) << "Parsed " << m_sqlStatements.size()
                        << " SQL statement(s) from " << m_inputFile << "\n";

    setupDbPath();

    m_results.reserve(m_sqlStatements.size());
    m_currentIndex = 0;

    // Kick off processing (let the event loop settle first)
    QTimer::singleShot(0, this, &TestRunner::processNext);
}

void TestRunner::processNext()
{
    m_results.append(executeSqlAndCapture(m_sqlStatements[m_currentIndex]));
    m_currentIndex++;

    if (m_currentIndex < m_sqlStatements.size()) {
        QTimer::singleShot(0, this, &TestRunner::processNext);
    } else {
        writeResults();
        QApplication::quit();
    }
}

void TestRunner::setupDbPath()
{
    QPushButton *setPathBtn = m_mainWindow->findChild<QPushButton *>("SetPath");
    if (!setPathBtn) {
        QTextStream(stderr) << "Warning: SetPath button not found\n";
        return;
    }

    // Trigger on_SetPath_clicked which creates a non-modal Dialog
    setPathBtn->click();

    // The Dialog is created as a child of MainWindow
    Dialog *dialog = m_mainWindow->findChild<Dialog *>();
    if (!dialog) {
        QTextStream(stderr) << "Warning: SetPath Dialog not found\n";
        return;
    }

    // Inject our path and accept – this triggers the lambda in
    // on_SetPath_clicked that sets MainWindow's private DBpath.
    QLineEdit *lineEdit = dialog->findChild<QLineEdit *>("lineEdit");
    if (lineEdit) {
        lineEdit->setText(m_dbRootPath);
    }
    dialog->accept();
}

QStringList TestRunner::parseSqlFile(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream(stderr) << "Error: Cannot open input file " << filePath << "\n";
        return {};
    }

    QTextStream in(&file);
    QStringList statements;
    QString current;
    bool inBlockComment = false;

    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();

        // Skip multi-line block comments (/* ... */)
        if (inBlockComment) {
            int endIdx = line.indexOf("*/");
            if (endIdx >= 0) {
                line = line.mid(endIdx + 2);
                inBlockComment = false;
            } else {
                continue;
            }
        }

        // Handle /* */ on a single line
        int startIdx = line.indexOf("/*");
        while (startIdx >= 0) {
            int endIdx = line.indexOf("*/", startIdx + 2);
            if (endIdx >= 0) {
                line = line.left(startIdx) + line.mid(endIdx + 2);
                startIdx = line.indexOf("/*");
            } else {
                inBlockComment = true;
                line = line.left(startIdx);
                break;
            }
        }

        // Skip empty lines and single-line comments
        if (line.isEmpty() || line.startsWith('#') || line.startsWith("--")) {
            continue;
        }

        // Strip inline -- comments (only outside of quoted strings)
        for (int i = 1; i < line.length(); ++i) {
            if (line[i - 1] == '-' && line[i] == '-') {
                int sq = line.left(i).count('\'');
                int dq = line.left(i).count('"');
                if (sq % 2 == 0 && dq % 2 == 0) {
                    line = line.left(i - 1).trimmed();
                    break;
                }
            }
        }

        if (line.isEmpty()) {
            continue;
        }

        current += line + " ";

        if (line.endsWith(';')) {
            QString stmt = current.trimmed();
            if (!stmt.isEmpty()) {
                statements.append(stmt);
            }
            current.clear();
        }
    }

    // Trailing statement without semicolon
    if (!current.trimmed().isEmpty()) {
        statements.append(current.trimmed());
    }

    file.close();
    return statements;
}

QString TestRunner::executeSqlAndCapture(const QString &sql)
{
    QTextEdit *terminal = m_mainWindow->findChild<QTextEdit *>("Terminal");
    QTextEdit *sqlEdit  = m_mainWindow->findChild<QTextEdit *>("sqlEdit");
    if (!terminal || !sqlEdit) {
        return "ERROR: Could not find Terminal or sqlEdit widget";
    }

    QString before = terminal->toPlainText();

    sqlEdit->setPlainText(sql);

    bool invoked = QMetaObject::invokeMethod(m_mainWindow,
                                             "on_SubmitSQL_clicked",
                                             Qt::DirectConnection);
    if (!invoked) {
        return "ERROR: Failed to invoke on_SubmitSQL_clicked";
    }

    QApplication::processEvents();

    QString after = terminal->toPlainText();

    // Diff the terminal content to extract what was just appended
    if (after.length() > before.length() && after.startsWith(before)) {
        return after.mid(before.length()).trimmed();
    }

    if (!before.isEmpty() && after.contains(before)) {
        int idx = after.indexOf(before) + before.length();
        return after.mid(idx).trimmed();
    }

    return after.isEmpty() ? "(no output)" : after.trimmed();
}

void TestRunner::writeResults()
{
    QFile file(m_outputFile);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream(stderr) << "Error: Cannot write output file " << m_outputFile << "\n";
        return;
    }

    QTextStream out(&file);

    out << "# DBMS Test Results\n\n";
    out << "| Field     | Value                          |\n";
    out << "|-----------|--------------------------------|\n";
    out << "| Input     | `" << m_inputFile << "` |\n";
    out << "| Time      | "
        << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << " |\n";
    out << "| Statements| " << m_sqlStatements.size() << " |\n";
    out << "\n---\n\n";

    int passed = 0;
    int failed = 0;

    for (int i = 0; i < m_sqlStatements.size(); ++i) {
        out << "### " << (i + 1) << ". SQL Statement\n\n";
        out << "```sql\n" << m_sqlStatements[i] << "\n```\n\n";
        out << "**Output:**\n\n```text\n" << m_results[i] << "\n```\n\n";
        out << "---\n\n";

        if (m_results[i].contains("失败") || m_results[i].contains("ERROR")) {
            failed++;
        } else {
            passed++;
        }
    }

    out << "## Summary\n\n";
    out << "| Status | Count |\n";
    out << "|--------|-------|\n";
    out << "| Total  | " << m_sqlStatements.size() << " |\n";
    out << "| Passed | " << passed << " |\n";
    out << "| Failed | " << failed << " |\n";

    file.close();

    QTextStream(stdout) << "Results written to " << m_outputFile << "\n";
    QTextStream(stdout) << "Passed: " << passed << "/" << m_sqlStatements.size() << "\n";
}
