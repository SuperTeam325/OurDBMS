#include "dialog.h"
#include "ui_dialog.h"
#include <QFileDialog>
#include <QStandardPaths>
#include <QListWidgetItem>

Dialog::Dialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::Dialog)
{
    ui->setupUi(this);
}

Dialog::~Dialog()
{
    delete ui;
}

void Dialog::on_SelelctDir_Button_clicked()
{
    // 调用QFileDialog的静态方法getExistingDirectory，弹出系统目录选择窗口
    QString selectedDirPath = QFileDialog::getExistingDirectory(
        this,                  // 父窗口
        "选择目录",            // 窗口标题
        QStandardPaths::writableLocation(QStandardPaths::HomeLocation),  // 默认打开的目录（这里是用户主目录）
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks  // 选项：只显示目录，不解析符号链接
        );

    // 如果用户选择了目录，更新路径显示
    if (!selectedDirPath.isEmpty()) {
        QListWidgetItem* item = new QListWidgetItem(selectedDirPath);
        item->setToolTip(selectedDirPath);  // 鼠标放上去显示完整路径
        ui->lineEdit->setText(item->text());
    }

}

