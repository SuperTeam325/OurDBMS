/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 6.9.3
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QFrame>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QStackedWidget>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QTreeWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QWidget *centralwidget;
    QHBoxLayout *mainHorizontalLayout;
    QFrame *navFrame;
    QVBoxLayout *navLayout;
    QLabel *currentUserLabel;
    QSpacerItem *navSpacer1;
    QPushButton *btnDataMgmt;
    QPushButton *btnConsole;
    QPushButton *btnLogViewer;
    QSpacerItem *navSpacer2;
    QPushButton *btnSetPath;
    QStackedWidget *contentStack;
    QWidget *dataMgmtPage;
    QVBoxLayout *dataMgmtLayout;
    QFrame *dataToolbar;
    QHBoxLayout *dataToolbarLayout;
    QLabel *dataMgmtTitle;
    QSpacerItem *spacerItem;
    QPushButton *btnRefreshTree;
    QSplitter *dataSplitter;
    QTreeWidget *treeWidget;
    QStackedWidget *detailStack;
    QWidget *detailWelcomePage;
    QVBoxLayout *welcomeLayout;
    QLabel *welcomeLabel;
    QWidget *detailTableStructPage;
    QWidget *detailTableDataPage;
    QWidget *consolePage;
    QVBoxLayout *consoleLayout;
    QLabel *consoleTitleLabel;
    QTextEdit *Terminal;
    QHBoxLayout *sqlInputLayout;
    QTextEdit *sqlEdit;
    QPushButton *SubmitSQL;
    QWidget *logViewerPage;
    QVBoxLayout *logViewerLayout;
    QHBoxLayout *logHeaderLayout;
    QLabel *logTitleLabel;
    QSpacerItem *spacerItem1;
    QPushButton *btnRefreshLogs;
    QSplitter *logSplitter;
    QListWidget *logFileList;
    QWidget *logContentPanel;
    QVBoxLayout *logContentLayout;
    QHBoxLayout *logSearchLayout;
    QLineEdit *logSearchInput;
    QLabel *logEntryCountLabel;
    QTreeWidget *logContentTree;
    QMenuBar *menubar;
    QStatusBar *statusBar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName("MainWindow");
        MainWindow->resize(1400, 900);
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName("centralwidget");
        mainHorizontalLayout = new QHBoxLayout(centralwidget);
        mainHorizontalLayout->setSpacing(0);
        mainHorizontalLayout->setObjectName("mainHorizontalLayout");
        mainHorizontalLayout->setContentsMargins(0, 0, 0, 0);
        navFrame = new QFrame(centralwidget);
        navFrame->setObjectName("navFrame");
        navFrame->setMinimumSize(QSize(180, 0));
        navFrame->setStyleSheet(QString::fromUtf8("QFrame#navFrame { background-color: #2c3e50; border-right: 1px solid #1a252f; }"));
        navLayout = new QVBoxLayout(navFrame);
        navLayout->setSpacing(2);
        navLayout->setObjectName("navLayout");
        navLayout->setContentsMargins(10, 20, 10, 20);
        currentUserLabel = new QLabel(navFrame);
        currentUserLabel->setObjectName("currentUserLabel");
        currentUserLabel->setStyleSheet(QString::fromUtf8("color: #ecf0f1; font-size: 12pt; font-weight: bold; padding: 10px 5px; border-bottom: 1px solid #34495e;"));

        navLayout->addWidget(currentUserLabel);

        navSpacer1 = new QSpacerItem(20, 15, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Fixed);

        navLayout->addItem(navSpacer1);

        btnDataMgmt = new QPushButton(navFrame);
        btnDataMgmt->setObjectName("btnDataMgmt");
        btnDataMgmt->setCheckable(true);
        btnDataMgmt->setChecked(true);
        btnDataMgmt->setMinimumSize(QSize(0, 42));
        btnDataMgmt->setStyleSheet(QString::fromUtf8("QPushButton { text-align: left; color: #ecf0f1; background-color: transparent; border: none; border-radius: 6px; font-size: 11pt; padding-left: 12px; } QPushButton:hover { background-color: #34495e; } QPushButton:checked { background-color: #3498db; color: white; font-weight: bold; }"));

        navLayout->addWidget(btnDataMgmt);

        btnConsole = new QPushButton(navFrame);
        btnConsole->setObjectName("btnConsole");
        btnConsole->setCheckable(true);
        btnConsole->setMinimumSize(QSize(0, 42));
        btnConsole->setStyleSheet(QString::fromUtf8("QPushButton { text-align: left; color: #ecf0f1; background-color: transparent; border: none; border-radius: 6px; font-size: 11pt; padding-left: 12px; } QPushButton:hover { background-color: #34495e; } QPushButton:checked { background-color: #3498db; color: white; font-weight: bold; }"));

        navLayout->addWidget(btnConsole);

        btnLogViewer = new QPushButton(navFrame);
        btnLogViewer->setObjectName("btnLogViewer");
        btnLogViewer->setCheckable(true);
        btnLogViewer->setMinimumSize(QSize(0, 42));
        btnLogViewer->setStyleSheet(QString::fromUtf8("QPushButton { text-align: left; color: #ecf0f1; background-color: transparent; border: none; border-radius: 6px; font-size: 11pt; padding-left: 12px; } QPushButton:hover { background-color: #34495e; } QPushButton:checked { background-color: #3498db; color: white; font-weight: bold; }"));

        navLayout->addWidget(btnLogViewer);

        navSpacer2 = new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        navLayout->addItem(navSpacer2);

        btnSetPath = new QPushButton(navFrame);
        btnSetPath->setObjectName("btnSetPath");
        btnSetPath->setMinimumSize(QSize(0, 38));
        btnSetPath->setStyleSheet(QString::fromUtf8("QPushButton { text-align: left; color: #bdc3c7; background-color: transparent; border: 1px solid #34495e; border-radius: 6px; font-size: 10pt; padding-left: 12px; } QPushButton:hover { background-color: #34495e; color: white; }"));

        navLayout->addWidget(btnSetPath);


        mainHorizontalLayout->addWidget(navFrame);

        contentStack = new QStackedWidget(centralwidget);
        contentStack->setObjectName("contentStack");
        dataMgmtPage = new QWidget();
        dataMgmtPage->setObjectName("dataMgmtPage");
        dataMgmtLayout = new QVBoxLayout(dataMgmtPage);
        dataMgmtLayout->setSpacing(6);
        dataMgmtLayout->setObjectName("dataMgmtLayout");
        dataMgmtLayout->setContentsMargins(10, 10, 10, 10);
        dataToolbar = new QFrame(dataMgmtPage);
        dataToolbar->setObjectName("dataToolbar");
        dataToolbar->setMaximumSize(QSize(16777215, 44));
        dataToolbarLayout = new QHBoxLayout(dataToolbar);
        dataToolbarLayout->setSpacing(8);
        dataToolbarLayout->setObjectName("dataToolbarLayout");
        dataToolbarLayout->setContentsMargins(5, 0, 5, 0);
        dataMgmtTitle = new QLabel(dataToolbar);
        dataMgmtTitle->setObjectName("dataMgmtTitle");
        dataMgmtTitle->setStyleSheet(QString::fromUtf8("font-size: 13pt; font-weight: bold; color: #2c3e50;"));

        dataToolbarLayout->addWidget(dataMgmtTitle);

        spacerItem = new QSpacerItem(0, 0, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        dataToolbarLayout->addItem(spacerItem);

        btnRefreshTree = new QPushButton(dataToolbar);
        btnRefreshTree->setObjectName("btnRefreshTree");
        btnRefreshTree->setMinimumSize(QSize(80, 32));
        btnRefreshTree->setStyleSheet(QString::fromUtf8("QPushButton { background-color: #3498db; color: white; border: none; border-radius: 4px; font-size: 10pt; padding: 4px 12px; } QPushButton:hover { background-color: #2980b9; }"));

        dataToolbarLayout->addWidget(btnRefreshTree);


        dataMgmtLayout->addWidget(dataToolbar);

        dataSplitter = new QSplitter(dataMgmtPage);
        dataSplitter->setObjectName("dataSplitter");
        dataSplitter->setOrientation(Qt::Horizontal);
        treeWidget = new QTreeWidget(dataSplitter);
        treeWidget->setObjectName("treeWidget");
        treeWidget->setMinimumSize(QSize(280, 0));
        treeWidget->setStyleSheet(QString::fromUtf8("QTreeWidget { border: 1px solid #dcdcdc; border-radius: 4px; font-size: 10pt; outline: none; } QTreeWidget:focus { border: 1px solid #dcdcdc; outline: none; } QTreeWidget::item { padding: 6px; border-bottom: 1px solid #ecf0f1; } QTreeWidget::item:selected { background-color: #3498db; color: white; } QTreeWidget::item:hover { background-color: #d4e6f1; } QTreeWidget::item:selected:hover { background-color: #2980b9; color: white; }"));
        dataSplitter->addWidget(treeWidget);
        detailStack = new QStackedWidget(dataSplitter);
        detailStack->setObjectName("detailStack");
        detailStack->setMinimumSize(QSize(400, 0));
        detailWelcomePage = new QWidget();
        detailWelcomePage->setObjectName("detailWelcomePage");
        welcomeLayout = new QVBoxLayout(detailWelcomePage);
        welcomeLayout->setObjectName("welcomeLayout");
        welcomeLabel = new QLabel(detailWelcomePage);
        welcomeLabel->setObjectName("welcomeLabel");
        welcomeLabel->setAlignment(Qt::AlignCenter);
        welcomeLabel->setStyleSheet(QString::fromUtf8("color: #7f8c8d; font-size: 12pt;"));

        welcomeLayout->addWidget(welcomeLabel);

        detailStack->addWidget(detailWelcomePage);
        detailTableStructPage = new QWidget();
        detailTableStructPage->setObjectName("detailTableStructPage");
        detailStack->addWidget(detailTableStructPage);
        detailTableDataPage = new QWidget();
        detailTableDataPage->setObjectName("detailTableDataPage");
        detailStack->addWidget(detailTableDataPage);
        dataSplitter->addWidget(detailStack);

        dataMgmtLayout->addWidget(dataSplitter);

        contentStack->addWidget(dataMgmtPage);
        consolePage = new QWidget();
        consolePage->setObjectName("consolePage");
        consoleLayout = new QVBoxLayout(consolePage);
        consoleLayout->setSpacing(6);
        consoleLayout->setObjectName("consoleLayout");
        consoleLayout->setContentsMargins(10, 10, 10, 10);
        consoleTitleLabel = new QLabel(consolePage);
        consoleTitleLabel->setObjectName("consoleTitleLabel");
        consoleTitleLabel->setStyleSheet(QString::fromUtf8("font-size: 13pt; font-weight: bold; color: #2c3e50; padding-bottom: 2px;"));

        consoleLayout->addWidget(consoleTitleLabel);

        Terminal = new QTextEdit(consolePage);
        Terminal->setObjectName("Terminal");
        Terminal->setReadOnly(true);
        Terminal->setStyleSheet(QString::fromUtf8("QTextEdit { background-color: #1e1e1e; color: #d4d4d4; border: 1px solid #3c3c3c; border-radius: 4px; font-family: Consolas, \"Courier New\", monospace; font-size: 11pt; padding: 8px; }"));

        consoleLayout->addWidget(Terminal);

        sqlInputLayout = new QHBoxLayout();
        sqlInputLayout->setSpacing(8);
        sqlInputLayout->setObjectName("sqlInputLayout");
        sqlEdit = new QTextEdit(consolePage);
        sqlEdit->setObjectName("sqlEdit");
        sqlEdit->setMinimumSize(QSize(0, 100));
        sqlEdit->setMaximumSize(QSize(16777215, 140));
        sqlEdit->setStyleSheet(QString::fromUtf8("QTextEdit { border: 1px solid #bdc3c7; border-radius: 4px; font-family: Consolas, \"Courier New\", monospace; font-size: 11pt; padding: 6px; } QTextEdit:focus { border-color: #3498db; }"));

        sqlInputLayout->addWidget(sqlEdit);

        SubmitSQL = new QPushButton(consolePage);
        SubmitSQL->setObjectName("SubmitSQL");
        SubmitSQL->setMinimumSize(QSize(90, 100));
        SubmitSQL->setMaximumSize(QSize(90, 140));
        SubmitSQL->setStyleSheet(QString::fromUtf8("QPushButton { background-color: #27ae60; color: white; border: none; border-radius: 4px; font-size: 12pt; font-weight: bold; } QPushButton:hover { background-color: #219a52; } QPushButton:pressed { background-color: #1e8449; }"));

        sqlInputLayout->addWidget(SubmitSQL);


        consoleLayout->addLayout(sqlInputLayout);

        contentStack->addWidget(consolePage);
        logViewerPage = new QWidget();
        logViewerPage->setObjectName("logViewerPage");
        logViewerLayout = new QVBoxLayout(logViewerPage);
        logViewerLayout->setSpacing(6);
        logViewerLayout->setObjectName("logViewerLayout");
        logViewerLayout->setContentsMargins(10, 10, 10, 10);
        logHeaderLayout = new QHBoxLayout();
        logHeaderLayout->setSpacing(8);
        logHeaderLayout->setObjectName("logHeaderLayout");
        logTitleLabel = new QLabel(logViewerPage);
        logTitleLabel->setObjectName("logTitleLabel");
        logTitleLabel->setStyleSheet(QString::fromUtf8("font-size: 13pt; font-weight: bold; color: #2c3e50;"));

        logHeaderLayout->addWidget(logTitleLabel);

        spacerItem1 = new QSpacerItem(0, 0, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        logHeaderLayout->addItem(spacerItem1);

        btnRefreshLogs = new QPushButton(logViewerPage);
        btnRefreshLogs->setObjectName("btnRefreshLogs");
        btnRefreshLogs->setMinimumSize(QSize(110, 32));
        btnRefreshLogs->setStyleSheet(QString::fromUtf8("QPushButton { background-color: #3498db; color: white; border: none; border-radius: 4px; font-size: 10pt; padding: 4px 12px; } QPushButton:hover { background-color: #2980b9; }"));

        logHeaderLayout->addWidget(btnRefreshLogs);


        logViewerLayout->addLayout(logHeaderLayout);

        logSplitter = new QSplitter(logViewerPage);
        logSplitter->setObjectName("logSplitter");
        logSplitter->setOrientation(Qt::Horizontal);
        logFileList = new QListWidget(logSplitter);
        logFileList->setObjectName("logFileList");
        logFileList->setMinimumSize(QSize(200, 0));
        logFileList->setStyleSheet(QString::fromUtf8("QListWidget { border: 1px solid #dcdcdc; border-radius: 4px; font-size: 10pt; outline: none; } QListWidget:focus { border: 1px solid #dcdcdc; outline: none; } QListWidget::item { padding: 6px; border-bottom: 1px solid #ecf0f1; } QListWidget::item:selected { background-color: #3498db; color: white; } QListWidget::item:hover { background-color: #d4e6f1; } QListWidget::item:selected:hover { background-color: #2980b9; color: white; }"));
        logSplitter->addWidget(logFileList);
        logContentPanel = new QWidget(logSplitter);
        logContentPanel->setObjectName("logContentPanel");
        logContentLayout = new QVBoxLayout(logContentPanel);
        logContentLayout->setSpacing(4);
        logContentLayout->setObjectName("logContentLayout");
        logContentLayout->setContentsMargins(0, 0, 0, 0);
        logSearchLayout = new QHBoxLayout();
        logSearchLayout->setSpacing(6);
        logSearchLayout->setObjectName("logSearchLayout");
        logSearchInput = new QLineEdit(logContentPanel);
        logSearchInput->setObjectName("logSearchInput");
        logSearchInput->setClearButtonEnabled(true);
        logSearchInput->setStyleSheet(QString::fromUtf8("QLineEdit { border: 1px solid #bdc3c7; border-radius: 4px; font-size: 10pt; padding: 5px 8px; } QLineEdit:focus { border-color: #3498db; }"));

        logSearchLayout->addWidget(logSearchInput);

        logEntryCountLabel = new QLabel(logContentPanel);
        logEntryCountLabel->setObjectName("logEntryCountLabel");
        logEntryCountLabel->setStyleSheet(QString::fromUtf8("color: #7f8c8d; font-size: 9pt; min-width: 60px;"));

        logSearchLayout->addWidget(logEntryCountLabel);


        logContentLayout->addLayout(logSearchLayout);

        logContentTree = new QTreeWidget(logContentPanel);
        logContentTree->setObjectName("logContentTree");
        logContentTree->setRootIsDecorated(true);
        logContentTree->setAnimated(true);
        logContentTree->setColumnCount(1);
        logContentTree->setStyleSheet(QString::fromUtf8("QTreeWidget { border: 1px solid #dcdcdc; border-radius: 4px; font-family: Consolas, \"Courier New\", monospace; font-size: 10pt; outline: none; } QTreeWidget:focus { border: 1px solid #dcdcdc; outline: none; } QTreeWidget::item { padding: 4px 6px; border-bottom: 1px solid #f0f0f0; } QTreeWidget::item:selected { background-color: #3498db; color: white; } QTreeWidget::item:hover { background-color: #d4e6f1; } QTreeWidget::item:selected:hover { background-color: #2980b9; color: white; }"));

        logContentLayout->addWidget(logContentTree);

        logSplitter->addWidget(logContentPanel);

        logViewerLayout->addWidget(logSplitter);

        contentStack->addWidget(logViewerPage);

        mainHorizontalLayout->addWidget(contentStack);

        MainWindow->setCentralWidget(centralwidget);
        menubar = new QMenuBar(MainWindow);
        menubar->setObjectName("menubar");
        menubar->setGeometry(QRect(0, 0, 1400, 25));
        MainWindow->setMenuBar(menubar);
        statusBar = new QStatusBar(MainWindow);
        statusBar->setObjectName("statusBar");
        MainWindow->setStatusBar(statusBar);

        retranslateUi(MainWindow);

        contentStack->setCurrentIndex(0);
        detailStack->setCurrentIndex(0);


        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QCoreApplication::translate("MainWindow", "DBMS", nullptr));
        currentUserLabel->setText(QCoreApplication::translate("MainWindow", "\345\275\223\345\211\215\347\224\250\346\210\267: -", nullptr));
        btnDataMgmt->setText(QCoreApplication::translate("MainWindow", "  \346\225\260\346\215\256\347\256\241\347\220\206", nullptr));
        btnConsole->setText(QCoreApplication::translate("MainWindow", "  \346\216\247\345\210\266\345\217\260", nullptr));
        btnLogViewer->setText(QCoreApplication::translate("MainWindow", "  \346\227\245\345\277\227\346\237\245\347\234\213", nullptr));
        btnSetPath->setText(QCoreApplication::translate("MainWindow", "  \350\256\276\347\275\256\346\225\260\346\215\256\345\272\223\350\267\257\345\276\204", nullptr));
        dataMgmtTitle->setText(QCoreApplication::translate("MainWindow", "\346\225\260\346\215\256\345\272\223\347\256\241\347\220\206", nullptr));
        btnRefreshTree->setText(QCoreApplication::translate("MainWindow", "\345\210\267\346\226\260", nullptr));
        QTreeWidgetItem *___qtreewidgetitem = treeWidget->headerItem();
        ___qtreewidgetitem->setText(0, QCoreApplication::translate("MainWindow", "\346\225\260\346\215\256\345\272\223\345\257\274\350\210\252", nullptr));
        welcomeLabel->setText(QCoreApplication::translate("MainWindow", "\350\257\267\344\273\216\345\267\246\344\276\247\345\257\274\350\210\252\346\240\221\351\200\211\346\213\251\346\225\260\346\215\256\345\272\223\346\210\226\350\241\250\350\277\233\350\241\214\346\223\215\344\275\234\n"
"\n"
"\345\217\263\351\224\256\347\202\271\345\207\273\346\225\260\346\215\256\345\272\223\350\212\202\347\202\271\357\274\232\346\226\260\345\273\272\350\241\250\343\200\201\345\210\240\351\231\244\346\225\260\346\215\256\345\272\223\n"
"\345\217\263\351\224\256\347\202\271\345\207\273\350\241\250\350\212\202\347\202\271\357\274\232\346\237\245\347\234\213\347\273\223\346\236\204\343\200\201\346\237\245\347\234\213\346\225\260\346\215\256\343\200\201\344\277\256\346\224\271\343\200\201\345\210\240\351\231\244", nullptr));
        consoleTitleLabel->setText(QCoreApplication::translate("MainWindow", "SQL \346\216\247\345\210\266\345\217\260", nullptr));
        sqlEdit->setPlaceholderText(QCoreApplication::translate("MainWindow", "\345\234\250\346\255\244\350\276\223\345\205\245 SQL \350\257\255\345\217\245...", nullptr));
        SubmitSQL->setText(QCoreApplication::translate("MainWindow", "\346\211\247\350\241\214", nullptr));
        logTitleLabel->setText(QCoreApplication::translate("MainWindow", "SQL \346\223\215\344\275\234\346\227\245\345\277\227", nullptr));
        btnRefreshLogs->setText(QCoreApplication::translate("MainWindow", "\345\210\267\346\226\260\346\227\245\345\277\227\345\210\227\350\241\250", nullptr));
        logSearchInput->setPlaceholderText(QCoreApplication::translate("MainWindow", "\350\276\223\345\205\245\345\205\263\351\224\256\345\255\227\350\277\207\346\273\244\346\227\245\345\277\227...", nullptr));
        logEntryCountLabel->setText(QString());
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
