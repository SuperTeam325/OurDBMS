# DML模块整合说明文档

## 一、整合目标
将Lexer、Parser、DML模块连接，使SQL能够被正确执行并写入数据文件。

---

## 二、整体流程
SQL字符串 → Lexer → Parser → DML → 文件

---

## 三、具体整合步骤

### 1. 输入SQL
```cpp
QString sql;
```

---

### 2. Lexer处理
```cpp
Lexer lexer(sql);
auto tokens = lexer.tokenize();
```
说明：将SQL字符串拆分为Token列表

---

### 3. Parser解析
```cpp
Parser parser(tokens);
InsertStatement stmt = parser.parseInsert();
```
说明：将Token转换为结构体

---

### 4. 调用DML
```cpp
DML::executeInsert(db, stmt);
```
说明：
- Parser负责解析
- DML负责执行

---

### 5. DML内部执行
```cpp
auto rows = loadTableRows(...);
# 修改数据
saveTableRows(...);
```

---

## 四、关键整合点

1. Parser输出必须直接传给DML
2. 数据操作必须先在内存完成
3. 必须调用保存函数写回文件
4. 路径必须统一：db.path/tableName

---

## 五、完整执行链

SQL输入  
↓  
Lexer.tokenize()  
↓  
Parser.parse()  
↓  
DML.execute()  
↓  
loadTableRows()  
↓  
修改数据  
↓  
saveTableRows()  
↓  
写入文件  
