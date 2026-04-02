/********************************************************************************
** Form generated from reading UI file 'dialog.ui'
**
** Created by: Qt User Interface Compiler version 6.9.3
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_DIALOG_H
#define UI_DIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QAbstractButton>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>

QT_BEGIN_NAMESPACE

class Ui_Dialog
{
public:
    QDialogButtonBox *buttonBox;
    QLineEdit *lineEdit;
    QPushButton *SelelctDir_Button;

    void setupUi(QDialog *Dialog)
    {
        if (Dialog->objectName().isEmpty())
            Dialog->setObjectName("Dialog");
        Dialog->resize(408, 288);
        buttonBox = new QDialogButtonBox(Dialog);
        buttonBox->setObjectName("buttonBox");
        buttonBox->setGeometry(QRect(180, 240, 191, 32));
        buttonBox->setOrientation(Qt::Orientation::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::StandardButton::Cancel|QDialogButtonBox::StandardButton::Ok);
        buttonBox->setCenterButtons(false);
        lineEdit = new QLineEdit(Dialog);
        lineEdit->setObjectName("lineEdit");
        lineEdit->setGeometry(QRect(0, 90, 401, 51));
        QFont font;
        font.setPointSize(12);
        font.setBold(false);
        lineEdit->setFont(font);
        SelelctDir_Button = new QPushButton(Dialog);
        SelelctDir_Button->setObjectName("SelelctDir_Button");
        SelelctDir_Button->setGeometry(QRect(20, 230, 101, 41));
        QFont font1;
        font1.setPointSize(12);
        font1.setBold(true);
        SelelctDir_Button->setFont(font1);

        retranslateUi(Dialog);
        QObject::connect(buttonBox, &QDialogButtonBox::accepted, Dialog, qOverload<>(&QDialog::accept));
        QObject::connect(buttonBox, &QDialogButtonBox::rejected, Dialog, qOverload<>(&QDialog::reject));

        QMetaObject::connectSlotsByName(Dialog);
    } // setupUi

    void retranslateUi(QDialog *Dialog)
    {
        Dialog->setWindowTitle(QCoreApplication::translate("Dialog", "Dialog", nullptr));
        lineEdit->setText(QString());
        SelelctDir_Button->setText(QCoreApplication::translate("Dialog", "\351\200\211\346\213\251\347\233\256\345\275\225", nullptr));
    } // retranslateUi

};

namespace Ui {
    class Dialog: public Ui_Dialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_DIALOG_H
