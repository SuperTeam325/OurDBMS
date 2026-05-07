#include "src/ui/SettingsDialog.h"

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(QString::fromUtf8("设置"));
    resize(450, 350);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    m_tabWidget = new QTabWidget;

    // ---- General tab ----
    QWidget* generalTab = new QWidget;
    QFormLayout* generalLayout = new QFormLayout(generalTab);
    m_pathEdit = new QLineEdit;
    m_pathEdit->setPlaceholderText(QString::fromUtf8("数据库存储路径..."));
    m_pathEdit->setEnabled(false);
    generalLayout->addRow(QString::fromUtf8("数据路径:"), m_pathEdit);
    m_tabWidget->addTab(generalTab, QString::fromUtf8("通用"));

    // ---- Appearance tab ----
    QWidget* appearanceTab = new QWidget;
    QFormLayout* appearanceLayout = new QFormLayout(appearanceTab);
    m_fontSizeSpin = new QSpinBox;
    m_fontSizeSpin->setRange(8, 24);
    m_fontSizeSpin->setValue(11);
    m_fontSizeSpin->setEnabled(false);
    appearanceLayout->addRow(QString::fromUtf8("字体大小:"), m_fontSizeSpin);
    m_tabWidget->addTab(appearanceTab, QString::fromUtf8("外观"));

    // ---- Editor tab ----
    QWidget* editorTab = new QWidget;
    QFormLayout* editorLayout = new QFormLayout(editorTab);
    m_autoCompleteCheck = new QCheckBox(QString::fromUtf8("自动补全"));
    m_autoCompleteCheck->setEnabled(false);
    m_syntaxHighlightCheck = new QCheckBox(QString::fromUtf8("语法高亮"));
    m_syntaxHighlightCheck->setEnabled(false);
    editorLayout->addRow(m_autoCompleteCheck);
    editorLayout->addRow(m_syntaxHighlightCheck);
    QLabel* comingSoon = new QLabel(QString::fromUtf8("更多设置即将推出..."));
    comingSoon->setStyleSheet("color: #6c7086; font-style: italic;");
    editorLayout->addRow(comingSoon);
    m_tabWidget->addTab(editorTab, QString::fromUtf8("编辑器"));

    mainLayout->addWidget(m_tabWidget);

    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(m_buttonBox);
}
