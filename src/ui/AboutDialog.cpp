#include "src/ui/AboutDialog.h"

AboutDialog::AboutDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(QString::fromUtf8("关于 Mini DBMS"));
    setFixedSize(380, 240);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setSpacing(12);
    layout->setContentsMargins(30, 20, 30, 20);

    QLabel* titleLabel = new QLabel("Mini DBMS");
    titleLabel->setStyleSheet("font-size: 18pt; font-weight: bold; color: #cdd6f4;");
    titleLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(titleLabel);

    QLabel* versionLabel = new QLabel(QString::fromUtf8("版本 0.1"));
    versionLabel->setStyleSheet("font-size: 11pt; color: #a6adc8;");
    versionLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(versionLabel);

    QLabel* techLabel = new QLabel(QString::fromUtf8("基于 Qt 6.9.3 和 C++17 构建"));
    techLabel->setStyleSheet("font-size: 10pt; color: #6c7086;");
    techLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(techLabel);

    QLabel* descLabel = new QLabel(QString::fromUtf8("轻量级文件型关系数据库管理系统\n支持标准SQL子集、用户权限管理和可视化操作"));
    descLabel->setStyleSheet("font-size: 10pt; color: #a6adc8;");
    descLabel->setAlignment(Qt::AlignCenter);
    descLabel->setWordWrap(true);
    layout->addWidget(descLabel);

    layout->addStretch();

    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    layout->addWidget(buttonBox);
}
