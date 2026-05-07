#ifndef SQLEDITORPANEL_H
#define SQLEDITORPANEL_H

#include <QWidget>
#include <QTextEdit>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QShortcut>
#include <QStringList>

class SqlEditorPanel : public QWidget
{
    Q_OBJECT

public:
    explicit SqlEditorPanel(QWidget *parent = nullptr);

    QString sqlText() const;

public slots:
    void setSql(const QString& sql);
    void appendOutput(const QString& text);
    void clearOutput();
    void clearInput();
    void submitSql();

signals:
    void sqlSubmitted(const QString& sql);
    void pathChangeRequested();

private:
    QTextEdit* m_sqlEdit;
    QTextEdit* m_terminal;
    QPushButton* m_submitButton;
    QPushButton* m_setPathButton;
};

#endif // SQLEDITORPANEL_H
