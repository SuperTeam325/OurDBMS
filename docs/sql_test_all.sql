-- ============================================================================
-- 完整 SQL 测试脚本 — 覆盖当前 DBMS 支持的所有 SQL 语句
-- 使用场景：学校管理系统（student、course、enrollment、classroom）
-- 在 DBMS 的 SQL 编辑器中逐条或批量执行
-- ============================================================================

-- ============================================================================
-- Part 1: DCL — 登录认证（两种语法格式）
-- ============================================================================

-- 1.1 完整格式：LOGIN USER <user> IDENTIFIED BY <password>
LOGIN USER admin IDENTIFIED BY admin123;

-- 1.2 如果上面失败，尝试简化格式：LOGIN <user> <password>
-- LOGIN admin admin123;

-- ============================================================================
-- Part 2: DDL — 数据库操作
-- ============================================================================

-- 2.1 创建数据库
CREATE DATABASE test_school;

-- 2.2 切换到该数据库
USE test_school;

-- ============================================================================
-- Part 3: DDL — CREATE TABLE（覆盖所有数据类型 + 列级约束 + 表级约束 + FOREIGN KEY）
-- ============================================================================

-- 3.1 列级约束：PRIMARY KEY, AUTO_INCREMENT, NOT NULL, UNIQUE, DEFAULT
CREATE TABLE student (
    id INT PRIMARY KEY AUTO_INCREMENT,
    name VARCHAR(20) NOT NULL,
    email VARCHAR(50) UNIQUE,
    age INT DEFAULT 18,
    gender CHAR(1) DEFAULT 'M'
);

-- 3.2 表级约束：CONSTRAINT <name> PRIMARY KEY / UNIQUE
CREATE TABLE course (
    id INT PRIMARY KEY AUTO_INCREMENT,
    title VARCHAR(50) NOT NULL,
    credit FLOAT DEFAULT 2.0,
    teacher VARCHAR(20) NOT NULL,
    CONSTRAINT pk_course PRIMARY KEY (id),
    CONSTRAINT uq_title UNIQUE (title)
);

-- 3.3 两种 FOREIGN KEY 语法：匿名外键 + 命名外键（CONSTRAINT <name> FOREIGN KEY）
CREATE TABLE enrollment (
    id INT PRIMARY KEY AUTO_INCREMENT,
    student_id INT NOT NULL,
    course_id INT NOT NULL,
    score FLOAT,
    CONSTRAINT fk_student FOREIGN KEY (student_id) REFERENCES student(id),
    FOREIGN KEY (course_id) REFERENCES course(id)
);

-- 3.4 CHAR 固定长度类型 + 更多约束组合
CREATE TABLE classroom (
    id INT PRIMARY KEY AUTO_INCREMENT,
    room_code CHAR(6) NOT NULL UNIQUE,
    building VARCHAR(20) DEFAULT 'A教学楼',
    capacity INT DEFAULT 30
);

-- ============================================================================
-- Part 4: DML — INSERT（覆盖全部四种插入形式）
-- ============================================================================

-- 4.1 按位置插入单行（id 用 NULL 让 AUTO_INCREMENT 自动填充）
INSERT INTO student VALUES (NULL, '张三', 'zhangsan@test.com', 20, 'M');

-- 4.2 按列名插入单行
INSERT INTO student (name, email, age, gender) VALUES ('李四', 'lisi@test.com', 21, 'F');

-- 4.3 按列名多行插入
INSERT INTO student (name, email, age, gender) VALUES
    ('王五', 'wangwu@test.com', 19, 'M'),
    ('赵六', 'zhaoliu@test.com', 22, 'F');

-- 4.4 按位置多行插入
INSERT INTO student VALUES
    (NULL, '孙七', 'sunqi@test.com', 20, 'M'),
    (NULL, '周八', 'zhouba@test.com', 23, 'F');

-- 4.5 插入时省略含 DEFAULT 的列，验证默认值生效
INSERT INTO student (name, email) VALUES ('吴九', 'wujiu@test.com');

-- 4.6 course 表数据
INSERT INTO course (title, credit, teacher) VALUES
    ('数据库原理', 3.0, '王教授'),
    ('操作系统', 4.0, '李教授'),
    ('数据结构', 3.5, '赵教授');

-- 4.7 enrollment 表数据
INSERT INTO enrollment (student_id, course_id, score) VALUES
    (1, 1, 85.5),
    (1, 2, 90.0),
    (2, 1, 78.0),
    (3, 3, 92.5);

-- 4.8 classroom 表数据
INSERT INTO classroom (room_code, building, capacity) VALUES
    ('A301', '主教学楼', 60),
    ('B102', '理工楼', 45),
    ('C205', '主教学楼', 80);

-- ============================================================================
-- Part 5: DML — SELECT *（当前仅支持全表查询）
-- ============================================================================

SELECT * FROM student;
SELECT * FROM course;
SELECT * FROM enrollment;
SELECT * FROM classroom;

-- ============================================================================
-- Part 6: DML — UPDATE（单列更新 + 多列更新，WHERE 为必选项）
-- ============================================================================

-- 6.1 更新单列
UPDATE student SET age = 24 WHERE id = 4;

-- 6.2 更新多列
UPDATE student SET email = 'new_lisi@test.com', age = 22 WHERE id = 2;

-- 6.3 更新 FLOAT 类型列
UPDATE course SET credit = 3.5 WHERE id = 2;

-- 6.4 更新外键关联表
UPDATE enrollment SET score = 88.0 WHERE id = 1;

-- 6.5 验证
SELECT * FROM student;
SELECT * FROM course;
SELECT * FROM enrollment;

-- ============================================================================
-- Part 7: DML — DELETE（WHERE 为必选项）
-- ============================================================================

-- 7.1 删除指定行
DELETE FROM enrollment WHERE id = 4;

-- 7.2 验证
SELECT * FROM enrollment;

-- ============================================================================
-- Part 8: DDL — ALTER TABLE ADD COLUMN（单列 + 多列）
-- ============================================================================

-- 8.1 添加单列（带 DEFAULT）
ALTER TABLE student ADD phone VARCHAR(15) DEFAULT 'N/A';

-- 8.2 添加多列
ALTER TABLE student ADD addr VARCHAR(100), ADD birthday VARCHAR(10);

-- 8.3 验证
SELECT * FROM student;

-- ============================================================================
-- Part 9: DDL — ALTER TABLE MODIFY COLUMN（修改类型/约束）
-- ============================================================================

-- 9.1 修改列类型和 NOT NULL 约束
ALTER TABLE student MODIFY name VARCHAR(50) NOT NULL;

-- 9.2 修改 DEFAULT 值
ALTER TABLE student MODIFY age INT DEFAULT 20;

-- 9.3 验证
SELECT * FROM student;

-- ============================================================================
-- Part 10: DDL — ALTER TABLE CHANGE COLUMN（重命名列）
-- ============================================================================

-- 10.1 仅重命名（保持类型不变）
ALTER TABLE student CHANGE phone telephone VARCHAR(15);

-- 10.2 重命名 + 改类型 + 改约束
ALTER TABLE student CHANGE addr address VARCHAR(200) DEFAULT '未知';

-- 10.3 验证
SELECT * FROM student;

-- ============================================================================
-- Part 11: DDL — ALTER TABLE ADD CONSTRAINT（添加表级约束）
-- ============================================================================

-- 11.1 添加 UNIQUE 约束
ALTER TABLE student ADD CONSTRAINT uq_telephone UNIQUE (telephone);

-- 11.2 添加带 DEFAULT 值的列，再给它加约束（为后续 DROP 测试做准备）
ALTER TABLE student ADD score FLOAT DEFAULT 0.0;
ALTER TABLE student ADD CONSTRAINT def_score DEFAULT (score);

-- 11.3 验证
SELECT * FROM student;

-- ============================================================================
-- Part 12: DDL — ALTER TABLE DROP CONSTRAINT
-- ============================================================================

-- 12.1 DROP UNIQUE（需要约束名，在 Part 11 中创建）
ALTER TABLE student DROP UNIQUE uq_telephone;

-- 12.2 DROP DEFAULT（需要约束名，在 Part 11 中创建）
ALTER TABLE student DROP DEFAULT def_score;

-- 12.3 DROP PRIMARY KEY（不需要约束名）
ALTER TABLE course DROP PRIMARY KEY;

-- 12.4 DROP NOT NULL（todo 表稍后建）
-- 见 Part 13

-- ============================================================================
-- Part 13: DDL — 更多约束变体测试
-- ============================================================================

-- 13.1 创建带命名 NOT NULL 约束的表（用于测试 DROP NOT NULL）
CREATE TABLE todo (
    id INT PRIMARY KEY AUTO_INCREMENT,
    title VARCHAR(100) NOT NULL,
    done INT DEFAULT 0,
    CONSTRAINT nn_title NOT NULL (title),
    CONSTRAINT def_done DEFAULT (done)
);

-- 13.2 DROP NOT NULL
ALTER TABLE todo DROP NOT NULL nn_title;

-- 13.3 DROP AUTO_INCREMENT（创建带命名自增约束的表）
CREATE TABLE log_entry (
    id INT,
    msg VARCHAR(200),
    CONSTRAINT ai_id AUTO_INCREMENT (id),
    CONSTRAINT pk_log PRIMARY KEY (id)
);

-- 13.4 DROP AUTO_INCREMENT
ALTER TABLE log_entry DROP AUTO_INCREMENT ai_id;

-- 13.5 DROP DEFAULT
ALTER TABLE todo DROP DEFAULT def_done;

-- 13.6 验证
SELECT * FROM todo;
SELECT * FROM log_entry;

-- ============================================================================
-- Part 14: DDL — ALTER TABLE DROP COLUMN（单列 + 多列）
-- ============================================================================

-- 14.1 删除单列
ALTER TABLE student DROP COLUMN telephone;

-- 14.2 删除多列
ALTER TABLE student DROP COLUMN address, DROP COLUMN birthday, DROP COLUMN score;

-- 14.3 验证
SELECT * FROM student;

-- ============================================================================
-- Part 15: DCL — 用户管理
-- ============================================================================

-- 15.1 创建用户（密码用单引号包裹）
CREATE USER alice IDENTIFIED BY 'alice123';

-- 15.2 创建另一个用户（密码用双引号包裹）
CREATE USER bob IDENTIFIED BY "bob456";

-- 15.3 创建第三个用户（纯文本密码）
CREATE USER charlie IDENTIFIED BY charlie789;

-- ============================================================================
-- Part 16: DCL — GRANT 授权
-- ============================================================================

-- 16.1 单表单权限
GRANT SELECT ON test_school.student TO alice;

-- 16.2 单表多权限
GRANT SELECT, INSERT, UPDATE ON test_school.course TO alice;

-- 16.3 数据库级通配符（所有表的特定权限）
GRANT CREATE TABLE, DROP TABLE, ALTER ON test_school.* TO bob;

-- 16.4 全库全表全权限（含 admin 提升）
GRANT ALL PRIVILEGES ON *.* TO alice;

-- ============================================================================
-- Part 17: DCL — REVOKE 回收权限
-- ============================================================================

-- 17.1 回收单个权限
REVOKE INSERT ON test_school.course FROM alice;

-- 17.2 回收所有权限（同时降级 admin）
REVOKE ALL PRIVILEGES ON *.* FROM alice;

-- 17.3 回收通配符权限
REVOKE CREATE TABLE ON test_school.* FROM bob;

-- ============================================================================
-- Part 18: DCL — 删除用户
-- ============================================================================

-- 18.1 删除用户 charlie（无特殊权限）
DROP USER charlie;

-- 18.2 删除用户 bob
DROP USER bob;

-- ============================================================================
-- Part 19: DDL — DROP TABLE
-- ============================================================================

-- 19.1 删除表（无外键依赖）
DROP TABLE todo;
DROP TABLE log_entry;

-- 19.2 删除 classroom 表
DROP TABLE classroom;

-- 19.3 删除 enrollment 表
DROP TABLE enrollment;

-- 19.4 删除剩余业务表
DROP TABLE course;
DROP TABLE student;

-- ============================================================================
-- Part 20: DDL — DROP DATABASE
-- ============================================================================

-- 20.1 创建临时数据库用于删除演示
CREATE DATABASE test_temp;

-- 20.2 删除临时数据库
DROP DATABASE test_temp;

-- 20.3 删除主测试数据库（所有表已清空）
DROP DATABASE test_school;

-- ============================================================================
-- Part 21: DCL — 登出
-- ============================================================================

LOGOUT;

-- ============================================================================
-- 测试结束 — 语句类型覆盖清单
-- ============================================================================
-- DDL (11 种)
--   [x] CREATE DATABASE          [x] DROP DATABASE
--   [x] USE                       [x] CREATE TABLE
--   [x] ALTER TABLE ADD COLUMN    [x] ALTER TABLE MODIFY COLUMN
--   [x] ALTER TABLE CHANGE COLUMN [x] ALTER TABLE ADD CONSTRAINT
--   [x] ALTER TABLE DROP COLUMN   [x] ALTER TABLE DROP CONSTRAINT
--   [x] DROP TABLE
--
-- DML (4 种)
--   [x] INSERT (位置/列名 x 单行/多行 = 4 种组合)
--   [x] UPDATE (单列/多列)
--   [x] DELETE
--   [x] SELECT *
--
-- DCL (6 种)
--   [x] LOGIN (完整格式)
--   [x] LOGOUT
--   [x] CREATE USER
--   [x] DROP USER
--   [x] GRANT (单表/多权限/通配符/ALL PRIVILEGES)
--   [x] REVOKE
--
-- 数据类型 (4 种)
--   [x] INT    [x] FLOAT    [x] CHAR(n)    [x] VARCHAR(n)
--
-- 约束类型 (7 种，含 DROP 操作)
--   [x] PRIMARY KEY       [x] NOT NULL
--   [x] UNIQUE            [x] DEFAULT
--   [x] AUTO_INCREMENT    [x] FOREIGN KEY (匿名 + 命名)
--   [x] DROP PRIMARY KEY  [x] DROP NOT NULL
--   [x] DROP UNIQUE       [x] DROP DEFAULT
--   [x] DROP AUTO_INCREMENT
-- ============================================================================
