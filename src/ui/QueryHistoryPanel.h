#ifndef QUERYHISTORYPANEL_H
#define QUERYHISTORYPANEL_H

#include <QWidget>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QStringList>

class QueryHistoryPanel : public QWidget
{
    Q_OBJECT

public:
    explicit QueryHistoryPanel(QWidget *parent = nullptr);

public slots:
    void addQuery(const QString& sql);
    void clearHistory();

signals:
    void querySelected(const QString& sql);

private:
    QListWidget* m_historyList;
    QPushButton* m_clearButton;
    QStringList m_queries;
    static const int MAX_ENTRIES = 100;
};

#endif // QUERYHISTORYPANEL_H
