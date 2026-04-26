# DBMS 自动化测试工具使用说明

## 概述

本工具用于自动化测试 DBMS 的 SQL 执行功能。通过命令行启动，读取指定的测试文件（`.txt` 或 `.md`），逐条执行其中的 SQL 语句，并将每条语句及其执行结果保存到 Markdown 文件中。

## 工作原理

```
测试文件(.md) → TestRunner解析SQL → 逐条注入MainWindow → 捕获Terminal输出 → 输出结果(.md)
```

工具在后台完成以下步骤：

1. 初始化 DCL 权限系统（自动创建系统数据库和管理员账户）
2. 使用管理员账户 `admin / 123456` 自动登录（跳过登录窗口）
3. 在后台维护 MainWindow 实例（不显示界面）
4. 通过 Qt 信号/槽机制将 SQL 逐条设置为输入栏文本，模拟点击提交按钮
5. 捕获输出栏的新增结果
6. 将所有语句和结果写入 Markdown 文件

## 构建

### 前置条件

- Windows 系统
- Qt 6.x（Widgets 模块），安装路径如 `D:/Qt/6.9.3/msvc2022_64`
- MSVC 2022 编译工具链
- CMake 3.16+
- Ninja 构建工具

### 编译步骤

```bash
# 1. 打开 "Developer Command Prompt for VS 2022"（或手动运行 vcvars64.bat）

# 2. 配置
cmake -S test -B build/test_runner -G Ninja -DCMAKE_PREFIX_PATH=D:/Qt/6.9.3/msvc2022_64

# 3. 编译
cmake --build build/test_runner
```

编译产物为 `build/test_runner/DBMS_Test.exe`。

## 使用方法

### 命令行格式

```
DBMS_Test.exe <测试文件> [选项]
```

### 参数说明

| 参数 | 说明 |
|------|------|
| `<测试文件>` | 必填。包含 SQL 语句的测试文件路径（`.txt` 或 `.md`） |
| `-o, --output <文件>` | 输出结果文件路径。默认：`<输入文件名>_result.md` |
| `-d, --dbpath <路径>` | 数据库存储根目录。默认：`../../dataDB`（相对于可执行文件） |
| `-h, --help` | 显示帮助信息 |

### 使用示例

```bash
# 基本用法：运行测试，结果自动保存为 example_test_result.md
DBMS_Test.exe D:\GitRepo\OurDBMS\test\example_test.md

# 指定输出文件
DBMS_Test.exe my_tests.sql -o my_results.md

# 指定数据库存储路径
DBMS_Test.exe test.md -d D:\MyData\dataDB -o result.md
```

**注意**：运行前需确保 Qt DLL 在 PATH 中：
```bash
set PATH=D:\Qt\6.9.3\msvc2022_64\bin;%PATH%
```

## 测试文件编写规范

### 基本格式

- 文件编码：UTF-8
- 支持格式：`.txt`、`.md`
- 每条 SQL 语句以分号 `;` 结尾
- 支持多行 SQL 语句

### 注释

| 注释类型 | 语法 | 示例 |
|----------|------|------|
| 单行注释 | `--` 或 `#` | `-- 这是一条注释` |
| 块注释 | `/* */` | `/* 多行注释 */` |
| 行内注释 | `--`（不在引号内） | `SELECT * FROM t; -- 查询` |

### 示例测试文件

```sql
# 测试用例：数据库和表的创建

-- 1. 创建数据库
CREATE DATABASE testdb;

-- 2. 切换到数据库
USE testdb;

-- 3. 创建学生表（含主键和非空约束）
CREATE TABLE students (
    id INT PRIMARY KEY,
    name VARCHAR(50) NOT NULL,
    age INT,
    email VARCHAR(100)
);

-- 4. 创建课程表（含默认值）
CREATE TABLE courses (
    course_id INT PRIMARY KEY,
    title VARCHAR(100) NOT NULL,
    teacher VARCHAR(50) DEFAULT 'TBD'
);

-- 5. 修改表：添加列
ALTER TABLE students ADD phone VARCHAR(20);

-- 6. 修改表：添加约束
ALTER TABLE students ADD CONSTRAINT uq_email UNIQUE (email);

-- 7. 创建新用户（仅管理员可执行）
CREATE USER testuser IDENTIFIED BY 'pass123';
```

### 支持的 SQL 类型

| 类型 | 语句 | 说明 |
|------|------|------|
| DDL | `CREATE DATABASE` | 创建数据库 |
| DDL | `USE` | 切换数据库 |
| DDL | `CREATE TABLE` | 创建表 |
| DDL | `ALTER TABLE ... ADD` | 添加列或约束 |
| DDL | `ALTER TABLE ... DROP` | 删除列或约束 |
| DDL | `ALTER TABLE ... MODIFY` | 修改列 |
| DCL | `LOGIN` | 登录 |
| DCL | `LOGOUT` | 登出 |
| DCL | `CREATE USER` | 创建用户（需管理员） |
| DCL | `DROP USER` | 删除用户（需管理员） |
| DCL | `GRANT ... ON ... TO` | 授权（需管理员） |
| DCL | `REVOKE ... ON ... FROM` | 撤权（需管理员） |

### 注意事项

1. **数据库路径**：首次运行 `CREATE DATABASE` 前，工具会自动设置数据库存储路径（默认可执行文件上两级目录的 `dataDB`）
2. **管理员登录**：工具始终以 `admin` 身份运行，默认密码 `123456`
3. **测试顺序**：SQL 语句按文件中出现的顺序依次执行，后续语句可依赖前面的执行结果
4. **USE 前置**：CREATE TABLE 和 ALTER TABLE 等操作需要先 `USE` 目标数据库

## 输出结果说明

### 输出格式

输出为 Markdown 文件，包含以下内容：

- **头部信息**：输入文件路径、运行时间、语句总数
- **逐条结果**：每条 SQL 语句的代码块和执行输出
- **统计摘要**：通过/失败计数

### 示例输出

````markdown
# DBMS Test Results

| Field     | Value                          |
|-----------|--------------------------------|
| Input     | `example_test.md` |
| Time      | 2026-04-26 15:59:58 |
| Statements| 6 |

---

### 1. SQL Statement

```sql
CREATE DATABASE testdb;
```

**Output:**

```text
数据库创建成功
```

---

...

## Summary

| Status | Count |
|--------|-------|
| Total  | 6 |
| Passed | 6 |
| Failed | 0 |
````

### 通过/失败判定

- **通过**：输出中不包含 "失败" 或 "ERROR"
- **失败**：输出中包含 "失败" 或 "ERROR"

### 常见错误信息

| 输出信息 | 原因 |
|----------|------|
| `SQL执行失败：未指定数据库` | 未执行 USE 语句选择数据库 |
| `SQL执行失败：失败，表已存在！` | 重复创建同名表 |
| `SQL执行失败：失败，字段已存在！` | 重复添加同名字段 |
| `SQL执行失败：未登录` | 管理员登录状态丢失 |
| `SQL执行失败：只有管理员可以执行 ...` | 当前用户非管理员 |
| `ERROR: Could not find Terminal or sqlEdit widget` | MainWindow 初始化异常 |

## 技术架构

```
main.cpp
├── QApplication 初始化
├── DclFacade::initialize()     —— 初始化权限系统
├── DclFacade::login()          —— 管理员自动登录
├── MainWindow(隐藏)            —— 后台维护UI实例
└── TestRunner                  —— 测试执行引擎
    ├── parseSqlFile()          —— 解析测试文件
    ├── setupDbPath()           —— 设置数据库路径
    ├── executeSqlAndCapture()  —— 执行SQL并捕获结果
    └── writeResults()          —— 写入结果文件
```

- TestRunner 与 MainWindow 的交互通过 Qt 元对象系统完成：`findChild` 查找控件，`QMetaObject::invokeMethod` 调用私有槽函数
- 各条 SQL 通过 `QTimer::singleShot` 链式调度，确保事件循环正常运转
- 未修改 DBMS 项目中的任何现有代码
