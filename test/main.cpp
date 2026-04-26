#include "../DDL/mainwindow.h"
#include "../DCL/dcl_facade.h"

#include "test_runner.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QDir>
#include <QTextStream>
#include <QTimer>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QApplication::setApplicationName("DBMS Test Tool");

    // --- Command-line parsing ---
    QCommandLineParser parser;
    parser.setApplicationDescription("Automated SQL test runner for DBMS");
    parser.addHelpOption();

    parser.addPositionalArgument("input", "Input test file (.txt or .md) containing SQL statements");

    QCommandLineOption outputOpt(
        QStringList() << "o" << "output",
        "Output markdown file (default: <input>_result.md)",
        "file");
    parser.addOption(outputOpt);

    QCommandLineOption dbPathOpt(
        QStringList() << "d" << "dbpath",
        "Root path for database storage (default: ../../dataDB relative to executable)",
        "path");
    parser.addOption(dbPathOpt);

    parser.process(a);

    const QStringList args = parser.positionalArguments();
    if (args.isEmpty()) {
        QTextStream(stderr) << "Error: No input file specified.\n";
        parser.showHelp(1);
    }

    const QString inputFile = args.first();
    const QString outputFile = parser.isSet(outputOpt)
        ? parser.value(outputOpt)
        : inputFile.left(inputFile.lastIndexOf('.')) + "_result.md";

    // Default to the main project's dataDB/ (set via CMake compile definition).
    // Falls back to relative-path resolution when building standalone.
    const QString dbRootPath = parser.isSet(dbPathOpt)
        ? parser.value(dbPathOpt)
        :
#ifdef PROJECT_DATA_DB_PATH
        QStringLiteral(PROJECT_DATA_DB_PATH)
#else
        QDir::cleanPath(QCoreApplication::applicationDirPath() + "/../../dataDB")
#endif
    ;

    // --- Initialize DCL (same as DDL/main.cpp) ---
    DCL::DclFacade facade(dbRootPath);
    QString initError;
    if (!facade.initialize(initError)) {
        QTextStream(stderr) << "System initialization failed: " << initError << "\n";
        return -1;
    }

    // Programmatic admin login (skip login dialog)
    QString loginError;
    if (!facade.login("admin", "123456", loginError)) {
        QTextStream(stderr) << "Admin login failed: " << loginError << "\n";
        return -1;
    }

    QTextStream(stdout) << "Logged in as: " << facade.currentSession().username << "\n";

    // --- Create MainWindow (hidden – no need to show the UI) ---
    MainWindow w(&facade);
    // Don't call w.show() – keep it hidden

    // --- Create TestRunner and schedule the run ---
    TestRunner runner(&w, inputFile, outputFile, dbRootPath);
    QTimer::singleShot(0, &runner, &TestRunner::start);

    return a.exec();
}
