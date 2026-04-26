# DML功能说明文档

## 一、插入操作（INSERT）

### 1. 功能说明
插入操作用于向数据库表中添加新的数据记录。系统通过解析SQL语句，将数据写入内存结构，并最终保存到.tbf文件中。

### 2. 核心函数（示意）
```cpp
int DML::executeInsert(DataBase& db, InsertStatement& stmt)
{
    auto rows = loadTableRows(db, stmt.tableName);
    rows.push_back(stmt.values);
    saveTableRows(db, stmt.tableName, rows);
    return 1;
}
```

### 3. 单条数据插入示例
```sql
INSERT INTO student VALUES (1, 'Tom');
```

执行逻辑：
- 解析出一条数据
- 插入到内存中的数据列表
- 写入文件

### 4. 多条数据插入示例
```sql
INSERT INTO student VALUES (1, 'Tom'), (2, 'Alice'), (3, 'Bob');
```

执行逻辑：
- 解析出多组数据
- 循环插入到内存结构
- 最后统一写入文件

---

## 二、更新操作（UPDATE）

### 1. 功能说明
更新操作用于修改表中已有的数据。

### 2. 核心函数（示意）
```cpp
int DML::executeUpdate(DataBase& db, UpdateStatement& stmt)
{
    auto rows = loadTableRows(db, stmt.tableName);
    for(auto& row : rows)
    {
        if(matchCondition(row, stmt.condition))
        {
            row[stmt.columnIndex] = stmt.newValue;
        }
    }
    saveTableRows(db, stmt.tableName, rows);
    return 1;
}
```

### 3. 示例
```sql
UPDATE student SET name = 'Alice' WHERE id = 1;
```

执行逻辑：
- 遍历数据
- 找到满足条件的记录
- 修改字段值
- 保存数据

---

## 三、删除操作（DELETE）

### 1. 功能说明
删除操作用于移除表中满足条件的数据记录。

### 2. 核心函数（示意）
```cpp
int DML::executeDelete(DataBase& db, DeleteStatement& stmt)
{
    auto rows = loadTableRows(db, stmt.tableName);
    rows.erase(remove_if(rows.begin(), rows.end(),
        [&](auto& row){ return matchCondition(row, stmt.condition); }),
        rows.end());
    saveTableRows(db, stmt.tableName, rows);
    return 1;
}
```

### 3. 示例
```sql
DELETE FROM student WHERE id = 1;
```

执行逻辑：
- 遍历数据
- 删除符合条件的记录
- 写回文件
