#ifndef INDEX_H
#define INDEX_H

#include <QString>
#include <QVector>
#include <QMap>
#include <QList>
#include <QDataStream>
#include <QFile>
#include <QDir>
#include "DDL.h"

// B+树的阶（每个节点最多 ORDER-1 个键，溢出时分裂）
static const int BTREE_ORDER = 64;

// =============================================
// B+树节点（磁盘持久化格式）
// =============================================
struct BTreeNode {
    bool isLeaf;
    QVector<QString> keys;                // 键值列表
    QVector< QVector<int> > rowIdLists;   // 叶子节点：每个键对应的 rowId 列表
    QVector<qint64> children;             // 内部节点：子节点文件偏移量（keys.size() + 1 个）
    qint64 nextLeaf;                      // 叶子节点：下一个叶子节点偏移，-1 表示末尾

    BTreeNode() : isLeaf(true), nextLeaf(-1) {}

    void write(QDataStream& out) const {
        out << isLeaf;
        out << static_cast<qint32>(keys.size());
        for (const auto& k : keys) out << k;
        if (isLeaf) {
            out << static_cast<qint32>(rowIdLists.size());
            for (const auto& list : rowIdLists) {
                out << static_cast<qint32>(list.size());
                for (int id : list) out << static_cast<qint32>(id);
            }
            out << nextLeaf;
        } else {
            out << static_cast<qint32>(children.size());
            for (qint64 c : children) out << c;
        }
    }

    void read(QDataStream& in) {
        qint32 count;
        in >> isLeaf;
        in >> count;
        keys.resize(count);
        for (int i = 0; i < count; i++) in >> keys[i];
        if (isLeaf) {
            in >> count;
            rowIdLists.resize(count);
            for (int i = 0; i < count; i++) {
                qint32 listSize;
                in >> listSize;
                rowIdLists[i].resize(listSize);
                for (int j = 0; j < listSize; j++) {
                    qint32 id;
                    in >> id;
                    rowIdLists[i][j] = id;
                }
            }
            in >> nextLeaf;
        } else {
            in >> count;
            children.resize(count);
            for (int i = 0; i < count; i++) in >> children[i];
        }
    }

    // 在 keys 中二分查找 key，返回索引（若未找到则返回应插入位置）
    int findKeyPos(const QString& key) const {
        int lo = 0, hi = keys.size();
        while (lo < hi) {
            int mid = (lo + hi) / 2;
            if (keys[mid] < key)
                lo = mid + 1;
            else
                hi = mid;
        }
        return lo;
    }

    // 在 keys 中二分查找 key，返回索引；未找到返回 -1
    int searchKey(const QString& key) const {
        int lo = 0, hi = keys.size() - 1;
        while (lo <= hi) {
            int mid = (lo + hi) / 2;
            if (keys[mid] == key) return mid;
            if (keys[mid] < key)
                lo = mid + 1;
            else
                hi = mid - 1;
        }
        return -1;
    }

    // 找到 key 应在的子树索引（内部节点用）
    int findChildIndex(const QString& key) const {
        int lo = 0, hi = keys.size();
        while (lo < hi) {
            int mid = (lo + hi) / 2;
            if (keys[mid] <= key)
                lo = mid + 1;
            else
                hi = mid;
        }
        return lo;
    }
};

// =============================================
// B+树（持久化到磁盘文件）
// =============================================
class BPlusTree {
public:
    BPlusTree();
    ~BPlusTree();

    // --- 文件操作 ---
    bool open(const QString& filePath);
    bool create(const QString& filePath, bool unique);
    void close();

    // --- 数据操作 ---
    bool insert(const QString& key, int rowId);
    bool remove(const QString& key, int rowId);
    QVector<int> search(const QString& key) const;
    QVector<int> rangeSearch(const QString& low, const QString& high) const;
    QVector<int> scanAll() const;

    // --- 元数据 ---
    QString filePath() const { return m_filePath; }
    bool isUnique() const { return m_unique; }
    int size() const { return m_entryCount; }

private:
    // --- 内存节点管理 ---
    int  allocNode();
    BTreeNode& nodeAt(int idx);
    const BTreeNode& nodeAt(int idx) const;

    // --- 文件 I/O ---
    void loadFromFile();
    void saveToFile();

    // --- B+树算法 ---
    int  findLeaf(const QString& key) const;
    void insertIntoParent(int leftChildIdx, int rightChildIdx, const QString& key);
    void splitLeaf(int leafIdx);
    void splitInternal(int nodeIdx);

    QString m_filePath;
    QFile m_file;
    bool m_unique;
    int m_entryCount;
    int m_rootIdx;            // 根节点索引，-1 表示空树
    int m_firstLeafIdx;       // 第一个叶子节点索引

    QVector<BTreeNode> m_nodes;  // 所有节点（内存中的工作副本）
};

// =============================================
// 已加载索引的运行时信息
// =============================================
struct LoadedIndex {
    IndexMeta meta;
    BPlusTree* tree;
    QVector<int> fieldIndices;  // 索引列在表字段中的位置
};

// =============================================
// 索引管理器：管理单个表的所有索引
// =============================================
class IndexManager {
public:
    IndexManager(const QString& dbPath, const QString& tableName);
    ~IndexManager();

    // --- 索引生命周期 ---
    bool createIndex(const DDL::Table& table,
                     const IndexMeta& meta,
                     const QVector<QVector<QString>>& currentRows);
    bool dropIndex(const QString& indexName);
    bool loadIndex(const QString& indexName, const DDL::Table& table);
    void unloadIndex(const QString& indexName);
    void loadAllIndexes(const QList<IndexMeta>& metas, const DDL::Table& table);
    void closeAll();

    // --- 索引使用 ---
    BPlusTree* getIndex(const QString& indexName);
    QString findBestIndex(const QString& columnName) const;

    // 复合索引键值拼接
    static QString buildCompositeKey(const QVector<QString>& values);

    // --- 数据同步 ---
    void onInsert(const QVector<QString>& row, int rowId);
    void onDelete(const QVector<QString>& row, int rowId);
    void onUpdate(const QVector<QString>& oldRow,
                  const QVector<QString>& newRow, int rowId);

private:
    QString indexDir() const;
    QString indexFilePath(const QString& indexName) const;
    QString buildKey(const LoadedIndex& idx, const QVector<QString>& row) const;

    QString m_dbPath;
    QString m_tableName;
    QMap<QString, LoadedIndex> m_indexes;
};

#endif // INDEX_H
