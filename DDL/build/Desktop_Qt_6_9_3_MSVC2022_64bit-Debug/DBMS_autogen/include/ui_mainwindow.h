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
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QStackedWidget>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QTreeWidget>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QWidget *centralwidget;
    QStackedWidget *stackedWidget;
    QWidget *page;
    QTextEdit *Terminal;
    QTextEdit *sqlEdit;
    QLabel *label;
    QFrame *line;
    QFrame *line_2;
    QFrame *line_3;
    QFrame *line_4;
    QPushButton *SubmitSQL;
    QPushButton *SetPath;
    QTreeWidget *treeWidget;
    QWidget *page_3;
    QWidget *page_2;
    QMenuBar *menubar;
    QToolBar *toolBar;
    QStatusBar *statusBar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName("MainWindow");
        MainWindow->resize(1322, 976);
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName("centralwidget");
        stackedWidget = new QStackedWidget(centralwidget);
        stackedWidget->setObjectName("stackedWidget");
        stackedWidget->setGeometry(QRect(-20, -10, 1351, 941));
        page = new QWidget();
        page->setObjectName("page");
        Terminal = new QTextEdit(page);
        Terminal->setObjectName("Terminal");
        Terminal->setGeometry(QRect(490, 170, 831, 511));
        QFont font;
        font.setPointSize(12);
        Terminal->setFont(font);
        Terminal->setReadOnly(true);
        sqlEdit = new QTextEdit(page);
        sqlEdit->setObjectName("sqlEdit");
        sqlEdit->setGeometry(QRect(490, 680, 721, 221));
        QFont font1;
        font1.setPointSize(11);
        font1.setBold(true);
        sqlEdit->setFont(font1);
        label = new QLabel(page);
        label->setObjectName("label");
        label->setGeometry(QRect(480, 150, 69, 19));
        label->setFont(font1);
        line = new QFrame(page);
        line->setObjectName("line");
        line->setGeometry(QRect(540, 150, 791, 16));
        line->setFrameShape(QFrame::Shape::HLine);
        line->setFrameShadow(QFrame::Shadow::Sunken);
        line_2 = new QFrame(page);
        line_2->setObjectName("line_2");
        line_2->setGeometry(QRect(470, 160, 20, 751));
        line_2->setFrameShape(QFrame::Shape::VLine);
        line_2->setFrameShadow(QFrame::Shadow::Sunken);
        line_3 = new QFrame(page);
        line_3->setObjectName("line_3");
        line_3->setGeometry(QRect(480, 900, 851, 16));
        line_3->setFrameShape(QFrame::Shape::HLine);
        line_3->setFrameShadow(QFrame::Shadow::Sunken);
        line_4 = new QFrame(page);
        line_4->setObjectName("line_4");
        line_4->setGeometry(QRect(1320, 160, 20, 751));
        line_4->setFrameShape(QFrame::Shape::VLine);
        line_4->setFrameShadow(QFrame::Shadow::Sunken);
        SubmitSQL = new QPushButton(page);
        SubmitSQL->setObjectName("SubmitSQL");
        SubmitSQL->setGeometry(QRect(1210, 680, 111, 221));
        QFont font2;
        font2.setPointSize(12);
        font2.setBold(true);
        SubmitSQL->setFont(font2);
        SetPath = new QPushButton(page);
        SetPath->setObjectName("SetPath");
        SetPath->setGeometry(QRect(480, 20, 151, 41));
        SetPath->setFont(font2);
        treeWidget = new QTreeWidget(page);
        treeWidget->setObjectName("treeWidget");
        treeWidget->setGeometry(QRect(60, 150, 391, 761));
        stackedWidget->addWidget(page);
        page_3 = new QWidget();
        page_3->setObjectName("page_3");
        stackedWidget->addWidget(page_3);
        page_2 = new QWidget();
        page_2->setObjectName("page_2");
        stackedWidget->addWidget(page_2);
        MainWindow->setCentralWidget(centralwidget);
        menubar = new QMenuBar(MainWindow);
        menubar->setObjectName("menubar");
        menubar->setGeometry(QRect(0, 0, 1322, 25));
        MainWindow->setMenuBar(menubar);
        toolBar = new QToolBar(MainWindow);
        toolBar->setObjectName("toolBar");
        MainWindow->addToolBar(Qt::ToolBarArea::TopToolBarArea, toolBar);
        statusBar = new QStatusBar(MainWindow);
        statusBar->setObjectName("statusBar");
        MainWindow->setStatusBar(statusBar);

        retranslateUi(MainWindow);

        stackedWidget->setCurrentIndex(0);


        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QCoreApplication::translate("MainWindow", "MainWindow", nullptr));
        label->setText(QCoreApplication::translate("MainWindow", "SQL\345\214\272", nullptr));
        SubmitSQL->setText(QCoreApplication::translate("MainWindow", "\345\217\221\351\200\201", nullptr));
        SetPath->setText(QCoreApplication::translate("MainWindow", "\350\256\276\347\275\256\346\225\260\346\215\256\345\272\223\350\267\257\345\276\204", nullptr));
        QTreeWidgetItem *___qtreewidgetitem = treeWidget->headerItem();
        ___qtreewidgetitem->setText(0, QCoreApplication::translate("MainWindow", "\346\225\260\346\215\256\345\272\223\345\257\274\350\210\252", nullptr));
        toolBar->setWindowTitle(QCoreApplication::translate("MainWindow", "toolBar", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
