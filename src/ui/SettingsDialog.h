#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QTabWidget>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QLabel>

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = nullptr);

private:
    QTabWidget* m_tabWidget;
    QLineEdit* m_pathEdit;
    QSpinBox* m_fontSizeSpin;
    QCheckBox* m_autoCompleteCheck;
    QCheckBox* m_syntaxHighlightCheck;
    QDialogButtonBox* m_buttonBox;
};

#endif // SETTINGSDIALOG_H
