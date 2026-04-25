# DCL 模块支持的 SQL 语法说明

**总览**
- 支持的 DCL 语句：`LOGIN`, `LOGOUT`, `CREATE USER`, `DROP USER`, `GRANT`, `REVOKE`。
- 权限动作（用于 `GRANT` / `REVOKE`）包括：`SELECT`, `INSERT`, `UPDATE`, `DELETE`, `CREATE`, `ALTER`, `DROP`。
- 管理员限制：`CREATE USER`、`DROP USER`、`GRANT`、`REVOKE` 必须由管理员用户执行。

**入口**
- 在 UI 中：由主窗口的提交按钮将 SQL 发送到 `DclFacade::tryHandleSessionSql` 进行优先处理（参见 `DDL/mainwindow.cpp` 的 `on_SubmitSQL_clicked`）。
- 后端处理：`DclFacade::tryHandleSessionSql` 解析并分发到相应处理函数（例如 `handleCreateUserSql`）。

1. LOGIN
---------
用途：用户登录会话。支持两种写法。

语法：

- 简写：

  LOGIN username password;

- 完整：

  LOGIN USER username IDENTIFIED BY 'password';

示例：

  LOGIN alice 's3cr3t';

备注：登录成功后会更新会话上下文；密码可用单/双引号包裹。

2. LOGOUT
---------
用途：注销当前会话。

语法：

  LOGOUT;

示例：

  LOGOUT;

3. CREATE USER
---------------
用途：创建新用户（仅管理员可执行）。

语法：

  CREATE USER username IDENTIFIED BY 'password';

示例：

  CREATE USER bob IDENTIFIED BY 'p@ssw0rd';

语义与约束：
- 调用链：`DclFacade::handleCreateUserSql`(dcl语句入口) → `UserRepository::createUser`(写入用户表)（写入 `dataDB/sys/users` 表）。
- 新用户默认 `is_admin = 0`（普通用户）。
- 创建用户前必须处于登录状态且当前用户为管理员，否则返回错误 “只有管理员可以执行 CREATE USER”。

4. DROP USER
-------------
用途：删除已有用户（仅管理员可执行）。

语法：

  DROP USER username;

示例：

  DROP USER bob;

语义与约束：
- 不能删除管理员用户（会返回错误）。
- 不能删除当前登录用户。
- 成功删除后，会调用权限服务清理该用户的权限记录。

5. GRANT
---------
用途：授予目标用户对指定表的权限（仅管理员可执行）。

语法：

  GRANT <action[,action...]> ON tableName TO username;

示例：

  GRANT SELECT, INSERT ON employees TO alice;

语义：
- `<action>` 为文本形式，后端使用 `DclFacade::parseActionFromText` 解析为内部 `TableAction`。
- 必须先 `USE` 指定数据库（会话中当前数据库非空），否则返回错误 “请先 USE 数据库后再执行 GRANT”。

6. REVOKE
----------
用途：撤销目标用户的指定权限（仅管理员可执行）。

语法：

  REVOKE <action[,action...]> ON tableName FROM username;

示例：

  REVOKE INSERT ON employees FROM alice;

语义：与 `GRANT` 对应。

7. 与 DDL 集成的注意事项
-------------------------
- DCL 语句优先由 `DclFacade::tryHandleSessionSql` 处理；当该函数返回 true 时，表示 SQL 已被 DCL 消耗（无论成功或失败），UI 会据此输出结果并清空输入。
- 非 DCL SQL（例如 `CREATE TABLE`, `ALTER TABLE`, `CREATE DATABASE`, `USE`, `SELECT` 等）在通过 `tryHandleSessionSql` 后，将继续走授权检查 `DclFacade::authorizeSql`，再由 DDL 模块解析并执行。

示例会话
---------
1) 管理员创建用户：

  -- 管理员登录
  LOGIN admin '123456';
  CREATE USER alice IDENTIFIED BY 'alicepwd';

2) 授权并切换数据库：

  USE mydb;
  GRANT SELECT, INSERT ON employees TO alice;

3) 非管理员尝试创建用户（失败示例）：

  LOGIN alice 'alicepwd';
  CREATE USER mallory IDENTIFIED BY 'x';
  -- 返回: 只有管理员可以执行 CREATE USER

错误与返回
-----------
- DCL 方法在遇到错误时会通过 `error` 文本返回给调用方（例如 UI）。常见错误包括未登录、权限不足、目标用户不存在、用户已存在等。

参考实现位置
----------------
- `DclFacade::tryHandleSessionSql` / `handleCreateUserSql` / `handleDropUserSql` / `handleGrantSql` / `handleRevokeSql`（文件： `DCL/dcl_facade.cpp`）
- `UserRepository::createUser` / `deleteUser` / `getUser`（文件： `DCL/user_repository.cpp`）
- UI 入口： `DDL/mainwindow.cpp` 的 `MainWindow::on_SubmitSQL_clicked`