#include "logindialog.h"
#include "ui_logindialog.h"

LoginDialog::LoginDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::LoginDialog)
{
    ui->setupUi(this);
    ui->passWordLine->setEchoMode(QLineEdit::Password);
}

LoginDialog::~LoginDialog()
{
    delete ui;
}

QString LoginDialog::username() const
{
    return ui->nameLine->text().trimmed();
}

QString LoginDialog::password() const
{
    return ui->passWordLine->text();
}

void LoginDialog::clearPassword()
{
    ui->passWordLine->clear();
}

void LoginDialog::on_loginButton_clicked()
{
    accept();
}
