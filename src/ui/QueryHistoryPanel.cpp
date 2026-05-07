#include "src/ui/QueryHistoryPanel.h"

QueryHistoryPanel::QueryHistoryPanel(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_historyList = new QListWidget;
    m_historyList->setAlternatingRowColors(true);
    layout->addWidget(m_historyList, 1);

    m_clearButton = new QPushButton(QString::fromUtf8("清空历史"));
    layout->addWidget(m_clearButton);

    connect(m_historyList, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem* item) {
        emit querySelected(item->text());
    });

    connect(m_clearButton, &QPushButton::clicked, this, &QueryHistoryPanel::clearHistory);
}

void QueryHistoryPanel::addQuery(const QString& sql)
{
    if (sql.trimmed().isEmpty()) return;

    if (!m_queries.isEmpty() && m_queries.first() == sql)
        return;

    m_queries.prepend(sql);
    if (m_queries.size() > MAX_ENTRIES)
        m_queries.removeLast();

    m_historyList->insertItem(0, sql);
    if (m_historyList->count() > MAX_ENTRIES)
        delete m_historyList->takeItem(m_historyList->count() - 1);
}

void QueryHistoryPanel::clearHistory()
{
    m_queries.clear();
    m_historyList->clear();
}
