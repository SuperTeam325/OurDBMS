#include "mainwindow.h"
#include <QApplication>
#include <QMessageBox>
#include "../DCL/dcl_facade.h"
#include "../DCL/logindialog.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    DCL::DclFacade facade;
    QString initError;
    if (!facade.initialize(initError)) {
        QMessageBox::critical(nullptr, "系统初始化失败", initError);
        return -1;
    }

    while (true) {
        LoginDialog loginDialog;
        const int result = loginDialog.exec();
        if (result != QDialog::Accepted) {
            return 0;
        }

        QString loginError;
        if (facade.login(loginDialog.username(), loginDialog.password(), loginError)) {
            break;
        }

        QMessageBox::warning(nullptr, "登录失败", loginError);
    }

    MainWindow w(&facade);
    w.show();
    return a.exec();
}
