#include "src/ui/SqlEditorPanel.h"
#include <QFont>

SqlEditorPanel::SqlEditorPanel(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(4, 4, 4, 4);
    mainLayout->setSpacing(6);

    // Set path button
    m_setPathButton = new QPushButton(QString::fromUtf8("设置数据库路径"));
    m_setPathButton->setObjectName("SetPath");
    connect(m_setPathButton, &QPushButton::clicked, this, &SqlEditorPanel::pathChangeRequested);
    mainLayout->addWidget(m_setPathButton);

    // Terminal output (read-only)
    QLabel* outputLabel = new QLabel("SQL区");
    outputLabel->setStyleSheet("font-size: 11pt; font-weight: bold;");
    mainLayout->addWidget(outputLabel);

    m_terminal = new QTextEdit;
    m_terminal->setReadOnly(true);
    m_terminal->setObjectName("Terminal");
    m_terminal->setFont(QFont("Consolas", 12));
    mainLayout->addWidget(m_terminal, 1);

    // SQL input area
    QHBoxLayout* inputLayout = new QHBoxLayout;
    inputLayout->setSpacing(8);

    m_sqlEdit = new QTextEdit;
    m_sqlEdit->setObjectName("sqlEdit");
    m_sqlEdit->setFont(QFont("Consolas", 11));
    m_sqlEdit->setPlaceholderText(QString::fromUtf8("在此输入SQL语句..."));
    inputLayout->addWidget(m_sqlEdit, 1);

    m_submitButton = new QPushButton(QString::fromUtf8("发送"));
    m_submitButton->setObjectName("SubmitSQL");
    m_submitButton->setFixedSize(110, 60);
    connect(m_submitButton, &QPushButton::clicked, this, &SqlEditorPanel::submitSql);
    inputLayout->addWidget(m_submitButton);

    mainLayout->addLayout(inputLayout);

    // Ctrl+Enter shortcut
    QShortcut* shortcut = new QShortcut(QKeySequence("Ctrl+Return"), this);
    connect(shortcut, &QShortcut::activated, this, &SqlEditorPanel::submitSql);
    QShortcut* shortcut2 = new QShortcut(QKeySequence("Ctrl+Enter"), this);
    connect(shortcut2, &QShortcut::activated, this, &SqlEditorPanel::submitSql);
}

QString SqlEditorPanel::sqlText() const
{
    return m_sqlEdit->toPlainText().trimmed();
}

void SqlEditorPanel::setSql(const QString& sql)
{
    m_sqlEdit->setPlainText(sql);
}

void SqlEditorPanel::appendOutput(const QString& text)
{
    m_terminal->append(text);
}

void SqlEditorPanel::clearOutput()
{
    m_terminal->clear();
}

void SqlEditorPanel::clearInput()
{
    m_sqlEdit->clear();
}

void SqlEditorPanel::submitSql()
{
    QString sql = m_sqlEdit->toPlainText().trimmed();
    if (!sql.isEmpty()) {
        emit sqlSubmitted(sql);
    }
}
