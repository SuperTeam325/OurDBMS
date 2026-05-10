#include "Index.h"
#include <QDebug>
#include <QQueue>
#include <QSet>
#include <algorithm>

// =============================================
// BPlusTree 实现（内存 B+Tree + 磁盘持久化）
// =============================================

BPlusTree::BPlusTree()
    : m_unique(false), m_entryCount(0), m_rootIdx(-1), m_firstLeafIdx(-1)
{
}

BPlusTree::~BPlusTree()
{
    close();
}

int BPlusTree::allocNode()
{
    int idx = m_nodes.size();
    m_nodes.append(BTreeNode());
    return idx;
}

BTreeNode& BPlusTree::nodeAt(int idx)
{
    return m_nodes[idx];
}

const BTreeNode& BPlusTree::nodeAt(int idx) const
{
    return m_nodes[idx];
}

// ---- 文件 I/O ----

void BPlusTree::loadFromFile()
{
    m_nodes.clear();
    m_file.seek(0);

    QDataStream in(&m_file);
    in.setVersion(QDataStream::Qt_5_15);

    qint32 magic, version, order;
    in >> magic >> version >> order >> m_unique >> m_entryCount;

    qint64 rootOff, firstOff;
    in >> rootOff >> firstOff;

    // 读取所有节点，记录 file offset → node index 映射
    QMap<qint64, int> offToIdx;
    while (!in.atEnd()) {
        qint64 offset = m_file.pos();
        int idx = allocNode();
        nodeAt(idx).read(in);
        offToIdx[offset] = idx;
    }

    // 翻译文件偏移 → 节点索引
    m_rootIdx = offToIdx.value(rootOff, -1);
    m_firstLeafIdx = offToIdx.value(firstOff, -1);

    for (int i = 0; i < m_nodes.size(); i++) {
        BTreeNode& node = m_nodes[i];
        if (!node.isLeaf) {
            for (int j = 0; j < node.children.size(); j++) {
                node.children[j] = offToIdx.value(node.children[j], -1);
            }
        } else if (node.nextLeaf >= 0) {
            node.nextLeaf = offToIdx.value(node.nextLeaf, -1);
        }
    }
}

void BPlusTree::saveToFile()
{
    if (m_rootIdx < 0) return;

    // BFS 遍历，分配新偏移量，构建 idx → offset 映射
    QMap<int, qint64> idxToOff;
    QVector<int> order;
    QQueue<int> queue;
    QSet<int> visited;

    queue.enqueue(m_rootIdx);
    while (!queue.isEmpty()) {
        int idx = queue.dequeue();
        if (visited.contains(idx)) continue;
        visited.insert(idx);
        order.append(idx);

        const BTreeNode& node = nodeAt(idx);
        if (!node.isLeaf) {
            for (int child : node.children) {
                if (child >= 0 && !visited.contains(child))
                    queue.enqueue(child);
            }
        } else if (node.nextLeaf >= 0 && !visited.contains((int)node.nextLeaf)) {
            queue.enqueue((int)node.nextLeaf);
        }
    }

    // 计算 header 大小（先序列化一次 header 获取长度）
    QByteArray headerBa;
    {
        QDataStream hs(&headerBa, QIODevice::WriteOnly);
        hs.setVersion(QDataStream::Qt_5_15);
        hs << qint32(0x49445846) << qint32(1) << qint32(BTREE_ORDER)
           << m_unique << qint32(m_entryCount) << qint64(0) << qint64(0);
    }
    qint64 curOff = headerBa.size();

    for (int idx : order) {
        idxToOff[idx] = curOff;
        QByteArray nodeBa;
        {
            QDataStream ns(&nodeBa, QIODevice::WriteOnly);
            ns.setVersion(QDataStream::Qt_5_15);
            nodeAt(idx).write(ns);
        }
        curOff += nodeBa.size();
    }

    // 写入文件
    m_file.close();
    m_file.open(QIODevice::ReadWrite | QIODevice::Truncate);
    m_file.seek(0);

    QDataStream out(&m_file);
    out.setVersion(QDataStream::Qt_5_15);
    out << qint32(0x49445846) << qint32(1) << qint32(BTREE_ORDER)
        << m_unique << qint32(m_entryCount)
        << idxToOff.value(m_rootIdx, qint64(-1))
        << idxToOff.value(m_firstLeafIdx, qint64(-1));

    // 写入节点，翻译 child 索引 → 文件偏移
    for (int idx : order) {
        BTreeNode node = nodeAt(idx);
        if (!node.isLeaf) {
            for (int j = 0; j < node.children.size(); j++) {
                node.children[j] = idxToOff.value((int)node.children[j], qint64(-1));
            }
        } else if (node.nextLeaf >= 0) {
            node.nextLeaf = idxToOff.value((int)node.nextLeaf, qint64(-1));
        }
        node.write(out);
    }
}

bool BPlusTree::open(const QString& filePath)
{
    m_filePath = filePath;
    m_file.setFileName(filePath);
    if (!m_file.open(QIODevice::ReadWrite)) {
        return false;
    }
    loadFromFile();
    return true;
}

bool BPlusTree::create(const QString& filePath, bool unique)
{
    m_filePath = filePath;
    m_file.setFileName(filePath);
    if (!m_file.open(QIODevice::ReadWrite | QIODevice::Truncate)) {
        return false;
    }

    m_unique = unique;
    m_entryCount = 0;
    m_rootIdx = -1;
    m_firstLeafIdx = -1;
    m_nodes.clear();

    // 写入空 header
    QDataStream out(&m_file);
    out.setVersion(QDataStream::Qt_5_15);
    out << qint32(0x49445846) << qint32(1) << qint32(BTREE_ORDER)
        << m_unique << qint32(0) << qint64(-1) << qint64(-1);

    return true;
}

void BPlusTree::close()
{
    if (m_file.isOpen() && m_rootIdx >= 0) {
        saveToFile();
    }
    if (m_file.isOpen()) {
        m_file.close();
    }
    m_nodes.clear();
    m_rootIdx = -1;
    m_firstLeafIdx = -1;
    m_entryCount = 0;
}

// ---- 查找 ----

int BPlusTree::findLeaf(const QString& key) const
{
    if (m_rootIdx < 0) return -1;

    int idx = m_rootIdx;
    while (true) {
        const BTreeNode& node = nodeAt(idx);
        if (node.isLeaf) return idx;
        int childIdx = node.findChildIndex(key);
        if (childIdx < 0 || childIdx >= node.children.size()) return -1;
        idx = (int)node.children[childIdx];
    }
}

QVector<int> BPlusTree::search(const QString& key) const
{
    int leafIdx = findLeaf(key);
    if (leafIdx < 0) return {};

    const BTreeNode& leaf = nodeAt(leafIdx);
    int pos = leaf.searchKey(key);
    if (pos < 0) return {};
    return leaf.rowIdLists[pos];
}

QVector<int> BPlusTree::rangeSearch(const QString& low, const QString& high) const
{
    QVector<int> result;
    if (m_rootIdx < 0) return result;

    int leafIdx = findLeaf(low);
    if (leafIdx < 0) return result;

    while (leafIdx >= 0) {
        const BTreeNode& leaf = nodeAt(leafIdx);
        for (int i = 0; i < leaf.keys.size(); i++) {
            if (leaf.keys[i] > high) return result;
            if (leaf.keys[i] >= low) {
                result.append(leaf.rowIdLists[i]);
            }
        }
        leafIdx = (int)leaf.nextLeaf;
    }
    return result;
}

QVector<int> BPlusTree::scanAll() const
{
    QVector<int> result;
    if (m_firstLeafIdx < 0) return result;

    int leafIdx = m_firstLeafIdx;
    while (leafIdx >= 0) {
        const BTreeNode& leaf = nodeAt(leafIdx);
        for (const auto& list : leaf.rowIdLists) {
            result.append(list);
        }
        leafIdx = (int)leaf.nextLeaf;
    }
    return result;
}

// ---- 插入 ----

bool BPlusTree::insert(const QString& key, int rowId)
{
    if (m_rootIdx < 0) {
        // 空树：创建根叶子节点
        int rootIdx = allocNode();
        BTreeNode& root = nodeAt(rootIdx);
        root.isLeaf = true;
        root.keys.append(key);
        root.rowIdLists.append(QVector<int>{rowId});
        root.nextLeaf = -1;
        m_rootIdx = rootIdx;
        m_firstLeafIdx = rootIdx;
        m_entryCount = 1;
        return true;
    }

    // 找到叶子节点
    int leafIdx = findLeaf(key);
    if (leafIdx < 0) return false;

    BTreeNode& leaf = nodeAt(leafIdx);
    int pos = leaf.searchKey(key);

    if (pos >= 0) {
        // key 已存在
        if (m_unique) return false;
        if (leaf.rowIdLists[pos].contains(rowId)) return false;
        leaf.rowIdLists[pos].append(rowId);
    } else {
        pos = leaf.findKeyPos(key);
        leaf.keys.insert(pos, key);
        leaf.rowIdLists.insert(pos, {rowId});
    }
    m_entryCount++;

    // 检查是否需要分裂
    if (leaf.keys.size() >= BTREE_ORDER) {
        splitLeaf(leafIdx);
    }

    return true;
}

void BPlusTree::splitLeaf(int leafIdx)
{
    BTreeNode& leaf = nodeAt(leafIdx);
    int mid = leaf.keys.size() / 2;

    // 创建新右叶子
    int rightIdx = allocNode();
    BTreeNode& right = nodeAt(rightIdx);
    right.isLeaf = true;
    for (int i = mid; i < leaf.keys.size(); i++) {
        right.keys.append(leaf.keys[i]);
        right.rowIdLists.append(leaf.rowIdLists[i]);
    }
    right.nextLeaf = leaf.nextLeaf;

    // 截断左叶子
    leaf.keys.resize(mid);
    leaf.rowIdLists.resize(mid);
    leaf.nextLeaf = rightIdx;

    QString splitKey = right.keys[0];

    // 将 splitKey 和 rightIdx 插入父节点
    // 需要找到父节点。通过在树中搜索知道：leafIdx 是某个父节点的子节点
    // 向上遍历：维护父子关系
    insertIntoParent(leafIdx, rightIdx, splitKey);

    // 更新 firstLeaf（如果左叶子是第一个且分裂了）
    // firstLeaf 不变，因为左叶子仍在原位置
}

void BPlusTree::insertIntoParent(int leftChildIdx, int rightChildIdx, const QString& key)
{
    // 如果左子节点是根
    if (leftChildIdx == m_rootIdx) {
        int newRootIdx = allocNode();
        BTreeNode& newRoot = nodeAt(newRootIdx);
        newRoot.isLeaf = false;
        newRoot.keys.append(key);
        newRoot.children.append(leftChildIdx);
        newRoot.children.append(rightChildIdx);
        m_rootIdx = newRootIdx;
        return;
    }

    // 找到父节点（遍历树找到包含 leftChildIdx 作为子节点的内部节点）
    int parentIdx = -1;
    int childPos = -1;

    // 用栈做 DFS 查找父节点
    QVector<int> stack;
    stack.append(m_rootIdx);
    while (!stack.isEmpty()) {
        int idx = stack.back();
        stack.pop_back();
        const BTreeNode& node = nodeAt(idx);
        if (!node.isLeaf) {
            for (int i = 0; i < node.children.size(); i++) {
                if ((int)node.children[i] == leftChildIdx) {
                    parentIdx = idx;
                    childPos = i;
                    break;
                }
                stack.append((int)node.children[i]);
            }
        }
        if (parentIdx >= 0) break;
    }

    if (parentIdx < 0) return;  // 不应该发生

    BTreeNode& parent = nodeAt(parentIdx);
    int insertPos = parent.findKeyPos(key);
    parent.keys.insert(insertPos, key);
    parent.children.insert(insertPos + 1, rightChildIdx);

    // 检查父节点是否需要分裂
    if (parent.keys.size() >= BTREE_ORDER) {
        splitInternal(parentIdx);
    }
}

void BPlusTree::splitInternal(int nodeIdx)
{
    BTreeNode& node = nodeAt(nodeIdx);
    int mid = node.keys.size() / 2;
    QString midKey = node.keys[mid];

    // 创建右内部节点
    int rightIdx = allocNode();
    BTreeNode& right = nodeAt(rightIdx);
    right.isLeaf = false;
    for (int i = mid + 1; i < node.keys.size(); i++) {
        right.keys.append(node.keys[i]);
    }
    for (int i = mid + 1; i < node.children.size(); i++) {
        right.children.append(node.children[i]);
    }

    // 截断左节点
    node.keys.resize(mid);
    node.children.resize(mid + 1);

    // 上推 midKey 到父节点
    if (nodeIdx == m_rootIdx) {
        int newRootIdx = allocNode();
        BTreeNode& newRoot = nodeAt(newRootIdx);
        newRoot.isLeaf = false;
        newRoot.keys.append(midKey);
        newRoot.children.append(nodeIdx);
        newRoot.children.append(rightIdx);
        m_rootIdx = newRootIdx;
    } else {
        insertIntoParent(nodeIdx, rightIdx, midKey);
    }
}

// ---- 删除 ----

bool BPlusTree::remove(const QString& key, int rowId)
{
    if (m_rootIdx < 0) return false;

    int leafIdx = findLeaf(key);
    if (leafIdx < 0) return false;

    BTreeNode& leaf = nodeAt(leafIdx);
    int pos = leaf.searchKey(key);
    if (pos < 0) return false;

    int ridPos = leaf.rowIdLists[pos].indexOf(rowId);
    if (ridPos < 0) return false;

    leaf.rowIdLists[pos].removeAt(ridPos);

    if (leaf.rowIdLists[pos].isEmpty()) {
        // 删除空 key
        leaf.keys.removeAt(pos);
        leaf.rowIdLists.removeAt(pos);
    }

    m_entryCount--;

    // 简化处理：不合并节点（避免复杂逻辑，对性能和空间影响有限）
    // 如果根是叶子且变空，重置树
    if (m_rootIdx == leafIdx && leaf.keys.isEmpty()) {
        m_nodes.clear();
        m_rootIdx = -1;
        m_firstLeafIdx = -1;
    }

    return true;
}

// =============================================
// IndexManager 实现
// =============================================

IndexManager::IndexManager(const QString& dbPath, const QString& tableName)
    : m_dbPath(dbPath), m_tableName(tableName)
{
}

IndexManager::~IndexManager()
{
    closeAll();
}

QString IndexManager::indexDir() const
{
    return m_dbPath + "/" + m_tableName + "/";
}

QString IndexManager::indexFilePath(const QString& indexName) const
{
    return indexDir() + m_tableName + "_" + indexName + ".idx";
}

QString IndexManager::buildCompositeKey(const QVector<QString>& values)
{
    return values.join(QChar(0x1F));
}

QString IndexManager::buildKey(const LoadedIndex& idx, const QVector<QString>& row) const
{
    QVector<QString> vals;
    for (int fi : idx.fieldIndices) {
        if (fi >= 0 && fi < row.size())
            vals.append(row[fi]);
        else
            vals.append("");
    }
    return buildCompositeKey(vals);
}

bool IndexManager::createIndex(const DDL::Table& table,
                                const IndexMeta& meta,
                                const QVector<QVector<QString>>& currentRows)
{
    // 确保索引目录存在
    QString dir = indexDir();
    QDir d;
    if (!d.exists(dir)) {
        d.mkpath(dir);
    }

    // 计算字段索引
    QVector<int> fieldIndices;
    for (const QString& col : meta.columns) {
        int fi = table.getFieldIndex(col);
        if (fi < 0) return false;
        fieldIndices.append(fi);
    }

    // 收集并排序 (key, rowId) 对，用于批量构建
    QVector<QPair<QString, int>> entries;
    for (int rowId = 0; rowId < currentRows.size(); rowId++) {
        QVector<QString> vals;
        for (int fi : fieldIndices) {
            if (fi < currentRows[rowId].size())
                vals.append(currentRows[rowId][fi]);
            else
                vals.append("");
        }
        entries.append(qMakePair(buildCompositeKey(vals), rowId));
    }

    // 按 key 排序
    std::sort(entries.begin(), entries.end(),
              [](const QPair<QString, int>& a, const QPair<QString, int>& b) {
                  return a.first < b.first;
              });

    // 批量构建 B+树
    BPlusTree* tree = new BPlusTree();
    QString path = indexFilePath(meta.name);
    if (!tree->create(path, meta.unique)) {
        delete tree;
        return false;
    }

    for (const auto& entry : entries) {
        if (!tree->insert(entry.first, entry.second)) {
            tree->close();
            QFile::remove(path);
            delete tree;
            return false;
        }
    }

    tree->close();

    // 重新打开以供后续使用
    if (!tree->open(path)) {
        delete tree;
        return false;
    }

    LoadedIndex li;
    li.meta = meta;
    li.tree = tree;
    li.fieldIndices = fieldIndices;
    m_indexes[meta.name] = li;

    return true;
}

bool IndexManager::dropIndex(const QString& indexName)
{
    unloadIndex(indexName);
    QString path = indexFilePath(indexName);
    QFile f(path);
    if (f.exists()) {
        return f.remove();
    }
    return true;
}

bool IndexManager::loadIndex(const QString& indexName, const DDL::Table& table)
{
    if (m_indexes.contains(indexName)) return true;

    // 需要 IndexMeta 信息，loadIndex 单索引调用需要额外 meta
    // 由外部通过 loadAllIndexes 统一处理
    Q_UNUSED(table);
    return false;
}

void IndexManager::unloadIndex(const QString& indexName)
{
    if (m_indexes.contains(indexName)) {
        LoadedIndex& li = m_indexes[indexName];
        if (li.tree) {
            li.tree->close();
            delete li.tree;
        }
        m_indexes.remove(indexName);
    }
}

void IndexManager::loadAllIndexes(const QList<IndexMeta>& metas, const DDL::Table& table)
{
    for (const IndexMeta& meta : metas) {
        if (m_indexes.contains(meta.name)) continue;

        QString path = indexFilePath(meta.name);
        if (!QFile::exists(path)) continue;

        BPlusTree* tree = new BPlusTree();
        if (!tree->open(path)) {
            delete tree;
            continue;
        }

        QVector<int> fieldIndices;
        for (const QString& col : meta.columns) {
            fieldIndices.append(table.getFieldIndex(col));
        }

        LoadedIndex li;
        li.meta = meta;
        li.tree = tree;
        li.fieldIndices = fieldIndices;
        m_indexes[meta.name] = li;
    }
}

void IndexManager::closeAll()
{
    for (auto it = m_indexes.begin(); it != m_indexes.end(); ++it) {
        if (it.value().tree) {
            it.value().tree->close();
            delete it.value().tree;
        }
    }
    m_indexes.clear();
}

BPlusTree* IndexManager::getIndex(const QString& indexName)
{
    if (m_indexes.contains(indexName))
        return m_indexes[indexName].tree;
    return nullptr;
}

QString IndexManager::findBestIndex(const QString& columnName) const
{
    for (auto it = m_indexes.begin(); it != m_indexes.end(); ++it) {
        const LoadedIndex& li = it.value();
        if (li.meta.columns.size() == 1 && li.meta.columns[0] == columnName) {
            return li.meta.name;
        }
    }
    return QString();
}

void IndexManager::onInsert(const QVector<QString>& row, int rowId)
{
    for (auto it = m_indexes.begin(); it != m_indexes.end(); ++it) {
        LoadedIndex& li = it.value();
        QString key = buildKey(li, row);
        li.tree->insert(key, rowId);
    }
}

void IndexManager::onDelete(const QVector<QString>& row, int rowId)
{
    for (auto it = m_indexes.begin(); it != m_indexes.end(); ++it) {
        LoadedIndex& li = it.value();
        QString key = buildKey(li, row);
        li.tree->remove(key, rowId);
    }
}

void IndexManager::onUpdate(const QVector<QString>& oldRow,
                             const QVector<QString>& newRow, int rowId)
{
    for (auto it = m_indexes.begin(); it != m_indexes.end(); ++it) {
        LoadedIndex& li = it.value();
        QString oldKey = buildKey(li, oldRow);
        QString newKey = buildKey(li, newRow);
        if (oldKey != newKey) {
            li.tree->remove(oldKey, rowId);
            li.tree->insert(newKey, rowId);
        }
    }
}
