-- ============================================================================
-- DCL 模块完整功能测试脚本
-- 可直接自上而下依次执行，无需手动干预
-- 覆盖：LOGIN/LOGOUT、CREATE/DROP USER、GRANT/REVOKE、权限验证、边界条件
-- ============================================================================

-- ============================================================================
-- 阶段一：环境准备
-- ============================================================================

LOGIN USER admin IDENTIFIED BY admin123;

CREATE DATABASE dcl_db;
USE dcl_db;

CREATE TABLE items (
    id INT PRIMARY KEY AUTO_INCREMENT,
    title VARCHAR(50) NOT NULL,
    price FLOAT DEFAULT 0.0
);

INSERT INTO items (title, price) VALUES ('初始商品', 99.9);
SELECT * FROM items;

-- ============================================================================
-- 阶段二：创建测试用户（覆盖三种密码格式）
-- ============================================================================

-- 单引号密码
CREATE USER alice IDENTIFIED BY 'alice123';

-- 双引号密码
CREATE USER bob IDENTIFIED BY "bob456";

-- 纯文本密码
CREATE USER charlie IDENTIFIED BY charlie789;

-- 用户名含下划线
CREATE USER test_user_1 IDENTIFIED BY 'tu1_pass';

-- 验证重复创建被拒绝
CREATE USER alice IDENTIFIED BY 'dup';
-- 预期错误：用户已存在

-- ============================================================================
-- 阶段三：GRANT — 覆盖全部权限类型（18 种场景）
-- ============================================================================

-- 3.1 单表单权限
GRANT SELECT ON dcl_db.items TO alice;
GRANT INSERT ON dcl_db.items TO alice;
GRANT UPDATE ON dcl_db.items TO alice;
GRANT DELETE ON dcl_db.items TO alice;

-- 3.2 单表多权限一次性授予
GRANT SELECT, INSERT, UPDATE ON dcl_db.items TO bob;

-- 3.3 数据库级通配符权限（db.*）
GRANT CREATE TABLE ON dcl_db.* TO alice;
GRANT DROP TABLE ON dcl_db.* TO alice;
GRANT ALTER ON dcl_db.* TO alice;

-- 3.4 全局通配符权限（*.*）
GRANT CREATE DATABASE ON *.* TO bob;
GRANT DROP DATABASE ON *.* TO bob;
GRANT CREATE USER ON *.* TO bob;
GRANT DROP USER ON *.* TO bob;
GRANT GRANT ON *.* TO bob;
GRANT REVOKE ON *.* TO bob;

-- 3.5 ALL PRIVILEGES 全局授权（自动提升为管理员）
GRANT ALL PRIVILEGES ON *.* TO charlie;

-- 3.6 幂等性测试：重复授权不报错
GRANT SELECT ON dcl_db.items TO alice;
-- 预期：静默成功

-- 3.7 不支持的权限类型
GRANT EXECUTE ON dcl_db.items TO alice;
-- 预期错误：GRANT 失败：存在不支持的权限动作

-- 3.8 授权给不存在的用户
GRANT SELECT ON dcl_db.items TO ghost;
-- 预期错误：用户不存在

-- ============================================================================
-- 阶段四：权限验证 — 切换用户测试各层次权限匹配
-- ============================================================================

-- 4.1 alice：精确匹配权限（SELECT + INSERT + UPDATE + DELETE）
LOGOUT;
LOGIN USER alice IDENTIFIED BY alice123;
USE dcl_db;

-- 应成功：alice 有 SELECT 权限
SELECT * FROM items;

-- 应成功：alice 有 INSERT 权限
INSERT INTO items (title, price) VALUES ('alice 插入', 10.0);

-- 应成功：alice 有 UPDATE 权限
UPDATE items SET price = 11.0 WHERE id = 1;

-- 应成功：alice 有 DELETE 权限
DELETE FROM items WHERE id = 1;

-- 应成功：alice 有 CREATE TABLE ON dcl_db.* 权限
CREATE TABLE alice_tbl (col INT PRIMARY KEY);

-- 应成功：alice 有 ALTER ON dcl_db.* 权限
ALTER TABLE alice_tbl ADD col2 VARCHAR(10);

-- 应成功：alice 有 DROP TABLE ON dcl_db.* 权限
DROP TABLE alice_tbl;

-- 应失败：alice 没有 CREATE DATABASE 权限
CREATE DATABASE alice_should_fail;
-- 预期错误：权限不足

-- 应失败：alice 没有 CREATE USER 权限
CREATE USER bad_user IDENTIFIED BY 'bad';
-- 预期错误：权限不足

-- 4.2 bob：数据库级 + 全局通配符权限
LOGOUT;
LOGIN USER bob IDENTIFIED BY bob456;
USE dcl_db;

-- 应成功：bob 有 SELECT ON dcl_db.items
SELECT * FROM items;

-- 应成功：bob 有 INSERT ON dcl_db.items
INSERT INTO items (title, price) VALUES ('bob 插入', 20.0);

-- 应失败：bob 没有 DELETE 权限
DELETE FROM items WHERE id = 1;
-- 预期错误：权限不足

-- 应成功：bob 有 CREATE DATABASE ON *.* 权限
CREATE DATABASE bob_db;

-- 应成功：bob 有 CREATE USER ON *.* 权限（但受限于 PermissionService 内部 admin 检查）
-- 注意：GRANT/REVOKE/CREATE USER/DROP USER 在 PermissionService 层要求 session.isAdmin，
-- 仅有权限表记录不足以执行，故以下可能失败
CREATE USER bob_test IDENTIFIED BY 'bt';
-- 可能报错：只有管理员可以执行

-- 切换回 bob_db 删除之
USE dcl_db;
DROP DATABASE bob_db;
-- 应成功：bob 有 DROP DATABASE ON *.* 权限

-- 4.3 charlie：ALL PRIVILEGES 管理员权限
LOGOUT;
LOGIN USER charlie IDENTIFIED BY charlie789;
USE dcl_db;

-- 管理员可执行任何操作，无需显式授权
SELECT * FROM items;
INSERT INTO items (title, price) VALUES ('charlie 管理员插入', 30.0);
UPDATE items SET price = 35.0 WHERE id = 1;
DELETE FROM items WHERE id = 1;

CREATE TABLE charlie_tbl (x INT PRIMARY KEY, y VARCHAR(10));
INSERT INTO charlie_tbl VALUES (1, 'hello');
SELECT * FROM charlie_tbl;
ALTER TABLE charlie_tbl ADD z FLOAT DEFAULT 1.0;
DROP TABLE charlie_tbl;

CREATE DATABASE charlie_db;
USE charlie_db;
CREATE TABLE demo (a INT);
INSERT INTO demo VALUES (1);
SELECT * FROM demo;
USE dcl_db;
DROP DATABASE charlie_db;

CREATE USER charlie_user IDENTIFIED BY 'cu';
DROP USER charlie_user;

-- 4.4 test_user_1：无任何显式授权的全新用户（仅 SELECT 公有权限）
LOGOUT;
LOGIN USER test_user_1 IDENTIFIED BY tu1_pass;
USE dcl_db;

-- 应成功：SELECT 对任何登录用户公有
SELECT * FROM items;

-- 应失败：无 INSERT 权限
INSERT INTO items (title, price) VALUES ('应失败', 1.0);
-- 预期错误：权限不足

-- 应失败：无 UPDATE 权限
UPDATE items SET price = 1.0 WHERE id = 1;
-- 预期错误：权限不足

-- 应失败：无 DELETE 权限
DELETE FROM items WHERE id = 1;
-- 预期错误：权限不足

-- 应失败：无 CREATE TABLE 权限
CREATE TABLE fail_tbl (a INT);
-- 预期错误：权限不足

-- 应失败：无 CREATE DATABASE 权限
CREATE DATABASE fail_db;
-- 预期错误：权限不足

-- 应失败：无 CREATE USER 权限
CREATE USER fail_user IDENTIFIED BY 'f';
-- 预期错误：权限不足

-- 4.5 未登录状态验证
LOGOUT;

-- 应失败：未登录无法执行 SQL
SELECT * FROM items;
-- 预期错误：未登录，禁止执行 SQL

CREATE DATABASE no_login_db;
-- 预期错误：未登录，禁止执行 SQL

CREATE USER no_login_user IDENTIFIED BY 'n';
-- 预期错误：未登录

-- ============================================================================
-- 阶段五：REVOKE — 回收权限并验证
-- ============================================================================

LOGIN USER admin IDENTIFIED BY admin123;
USE dcl_db;

-- 5.1 回收 alice 的单权限
REVOKE DELETE ON dcl_db.items FROM alice;
REVOKE INSERT ON dcl_db.items FROM alice;

-- 5.2 回收 alice 的数据库级通配符权限
REVOKE CREATE TABLE ON dcl_db.* FROM alice;
REVOKE ALTER ON dcl_db.* FROM alice;

-- 5.3 回收 bob 的全局通配符权限
REVOKE CREATE DATABASE ON *.* FROM bob;
REVOKE DROP DATABASE ON *.* FROM bob;

-- 5.4 幂等性：回收不存在的权限静默成功
REVOKE DELETE ON dcl_db.items FROM alice;
-- 预期：静默成功

-- 5.5 验证回收后权限失效 — alice
LOGOUT;
LOGIN USER alice IDENTIFIED BY alice123;
USE dcl_db;

SELECT * FROM items;       -- 应成功（公有 + 仍有 SELECT 授权）

DELETE FROM items WHERE id = 1;
-- 预期错误：权限不足（DELETE 已被回收）

INSERT INTO items (title) VALUES ('应失败');
-- 预期错误：权限不足（INSERT 已被回收）

CREATE TABLE fail2 (a INT);
-- 预期错误：权限不足（CREATE TABLE 已被回收）

-- 5.6 验证回收后权限失效 — bob
LOGOUT;
LOGIN USER bob IDENTIFIED BY bob456;
USE dcl_db;

SELECT * FROM items;       -- 应成功（公有 + 仍有 SELECT 授权）
INSERT INTO items (title) VALUES ('bob 再插入');  -- 应成功

CREATE DATABASE fail_bob;
-- 预期错误：权限不足（CREATE DATABASE 已被回收）

-- 5.7 回收 ALL PRIVILEGES，降级管理员
LOGOUT;
LOGIN USER admin IDENTIFIED BY admin123;
USE dcl_db;

REVOKE ALL PRIVILEGES ON *.* FROM charlie;

-- 验证 charlie 降级
LOGOUT;
LOGIN USER charlie IDENTIFIED BY charlie789;
USE dcl_db;

SELECT * FROM items;       -- 应成功（公有权限）

CREATE USER fail_cu2 IDENTIFIED BY 'x';
-- 预期错误：权限不足（已不是管理员）

-- ============================================================================
-- 阶段六：用户登录格式测试
-- ============================================================================

LOGOUT;

-- 6.1 简化格式登录
LOGIN admin admin123;
USE dcl_db;
SELECT * FROM items;

-- 6.2 完整格式登录
LOGOUT;
LOGIN USER admin IDENTIFIED BY admin123;

-- 6.3 错误密码
LOGOUT;
LOGIN USER admin IDENTIFIED BY wrong_pwd;
-- 预期错误：用户名或密码错误

-- 6.4 不存在用户
LOGIN USER no_one IDENTIFIED BY x;
-- 预期错误：用户名或密码错误

-- 6.5 含空格密码（用引号包裹）
LOGIN USER admin IDENTIFIED BY admin123;
CREATE USER space_user IDENTIFIED BY 'pass with spaces';
LOGOUT;
LOGIN USER space_user IDENTIFIED BY 'pass with spaces';
-- 应成功
LOGOUT;
LOGIN USER admin IDENTIFIED BY admin123;

-- ============================================================================
-- 阶段七：DROP USER 边界条件测试
-- ============================================================================

USE dcl_db;

-- 7.1 带引号删除
CREATE USER quote_del IDENTIFIED BY 'qd';
DROP USER 'quote_del';

-- 7.2 删除不存在的用户
DROP USER no_such_user;
-- 预期错误：用户不存在

-- 7.3 不能删除自己
DROP USER admin;
-- 预期错误：删除失败：不允许删除当前登录用户

-- 7.4 不能删除管理员
-- 先创建一个管理员
CREATE USER admin2 IDENTIFIED BY 'a2';
GRANT ALL PRIVILEGES ON *.* TO admin2;

DROP USER admin2;
-- 预期错误：删除失败：不允许删除管理员用户

-- 清理 admin2 的管理员状态
REVOKE ALL PRIVILEGES ON *.* FROM admin2;
DROP USER admin2;

-- 7.5 非管理员不能删除用户
LOGOUT;
LOGIN USER alice IDENTIFIED BY alice123;
DROP USER bob;
-- 预期错误：权限不足

-- ============================================================================
-- 阶段八：权限隔离与层次匹配深度验证
-- ============================================================================

LOGOUT;
LOGIN USER admin IDENTIFIED BY admin123;
USE dcl_db;

-- 8.1 创建第二个测试数据库用于跨库隔离测试
CREATE DATABASE dcl_db2;
CREATE TABLE dcl_db2.other_items (id INT PRIMARY KEY AUTO_INCREMENT, note VARCHAR(50));

-- 8.2 授予 alice 仅在 dcl_db 上的权限
GRANT INSERT ON dcl_db.items TO alice;

-- 8.3 验证 alice 在 dcl_db2 上没有权限
LOGOUT;
LOGIN USER alice IDENTIFIED BY alice123;

USE dcl_db;
INSERT INTO items (title, price) VALUES ('alice 在 dcl_db', 50.0);
-- 应成功

USE dcl_db2;
INSERT INTO other_items (note) VALUES ('alice 尝试跨库');
-- 预期错误：权限不足（alice 的 INSERT 权限仅在 dcl_db.items 上）

-- 8.4 验证通配符权限的层次匹配
LOGOUT;
LOGIN USER admin IDENTIFIED BY admin123;

-- 授予 bob dcl_db.* 的 SELECT, INSERT 权限
GRANT SELECT, INSERT ON dcl_db.* TO bob;

LOGOUT;
LOGIN USER bob IDENTIFIED BY bob456;

USE dcl_db;
SELECT * FROM items;          -- 应成功（SELECT 公有 + db.* 匹配）
INSERT INTO items (title) VALUES ('bob 通配符');  -- 应成功（INSERT ON dcl_db.*）

USE dcl_db2;
SELECT * FROM other_items;    -- 应成功（SELECT 公有权限，不检查权限表）
INSERT INTO other_items (note) VALUES ('应失败');
-- 预期错误：权限不足（bob 的 db.* 权限只在 dcl_db 上）

-- 8.5 验证 *.* 全局通配符权限
LOGOUT;
LOGIN USER admin IDENTIFIED BY admin123;
GRANT DELETE ON *.* TO bob;

LOGOUT;
LOGIN USER bob IDENTIFIED BY bob456;

USE dcl_db;
DELETE FROM items WHERE id = 1;
-- 应成功（*.* DELETE 权限覆盖所有库所有表）

USE dcl_db2;
DELETE FROM other_items WHERE id = 1;
-- 应成功（*.* DELETE 权限覆盖所有库所有表）

-- ============================================================================
-- 阶段九：删除用户后权限清理验证
-- ============================================================================

LOGOUT;
LOGIN USER admin IDENTIFIED BY admin123;
USE dcl_db;

-- 9.1 创建临时用户并授予多种权限
CREATE USER cleanup_tester IDENTIFIED BY 'ct';
GRANT SELECT ON dcl_db.items TO cleanup_tester;
GRANT INSERT ON dcl_db.items TO cleanup_tester;
GRANT CREATE TABLE ON dcl_db.* TO cleanup_tester;

-- 9.2 删除该用户
DROP USER cleanup_tester;

-- 9.3 重建同名用户，验证旧权限不残留
CREATE USER cleanup_tester IDENTIFIED BY 'ct2';

LOGOUT;
LOGIN USER cleanup_tester IDENTIFIED BY ct2;
USE dcl_db;

SELECT * FROM items;              -- 应成功（SELECT 公有）
INSERT INTO items (title) VALUES ('清理后');
-- 预期错误：权限不足（旧权限已被清理）

CREATE TABLE fail_cleanup (a INT);
-- 预期错误：权限不足（旧权限已被清理）

-- ============================================================================
-- 阶段十：GRANT 大小写不敏感与旧格式兼容测试
-- ============================================================================

LOGOUT;
LOGIN USER admin IDENTIFIED BY admin123;
USE dcl_db;

-- 10.1 小写 grant（关键字大小写不敏感）
grant select, insert on dcl_db.items to alice;
-- 应成功

-- 10.2 旧格式：CREATE 代替 CREATE TABLE
GRANT CREATE ON dcl_db.* TO bob;
-- 应成功（内部映射为 CREATE TABLE）

-- 10.3 旧格式：DROP 代替 DROP TABLE
GRANT DROP ON dcl_db.* TO bob;
-- 应成功（内部映射为 DROP TABLE）

-- ============================================================================
-- 阶段十一：最终清理
-- ============================================================================

-- 删除测试用户
DROP USER alice;
DROP USER bob;
DROP USER charlie;
DROP USER test_user_1;
DROP USER space_user;
DROP USER cleanup_tester;

-- 删除测试数据库
DROP DATABASE dcl_db;
DROP DATABASE dcl_db2;

-- 登出
LOGOUT;

-- ============================================================================
-- 测试结束
-- ============================================================================
-- 覆盖清单：
--   LOGIN: 完整格式 / 简化格式 / 错误密码 / 不存在用户 / 引号密码
--   LOGOUT: 正常登出 / 未登录登出
--   CREATE USER: 单引号密码 / 双引号密码 / 纯文本密码 / 下划线用户名 / 重复创建
--   DROP USER: 正常删除 / 带引号 / 删除自己 / 删除管理员 / 不存在用户 / 非管理员
--   GRANT: SELECT / INSERT / UPDATE / DELETE / CREATE TABLE / DROP TABLE / ALTER
--          CREATE DATABASE / DROP DATABASE / CREATE USER / DROP USER
--          GRANT / REVOKE / ALL PRIVILEGES
--          单表 / 多权限 / db.* 通配 / *.* 全局 / 旧格式兼容 / 大小写 / 幂等
--   REVOKE: 单权限 / 多权限 / 通配符 / ALL PRIVILEGES / 幂等 / 降级验证
--   权限验证: 精确匹配 / db.* 层次 / *.* 全局 / SELECT 公有 / 未授权拒绝
--            跨库隔离 / 管理员旁路 / 未登录拒绝 / 删用户后清理
-- ============================================================================
