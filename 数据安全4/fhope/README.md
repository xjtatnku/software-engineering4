# FH-OPE 密文查询实验

本目录把指导书 7.3.3 的代码整理成可运行工程，并在 `client.py` 中加入了重复插入同一明文的测试模式。

## 文件说明

- `Node.h` / `Node.cpp`：FH-OPE 编码树实现。
- `UDF.cpp`：MySQL UDF 入口。
- `init.sql`：创建 `example` 表、UDF 函数和 `pro_insert` 存储过程。
- `client.py`：客户端加密、插入、查询和重复插入实验。
- `setup_wsl.sh`：在 WSL Ubuntu 中安装依赖、编译 UDF、初始化数据库。

## 一键准备环境

在 `Ubuntu-OSLab-Recovered` 中运行：

```bash
cd /mnt/c/Users/xjt26/Desktop/数据安全4/fhope
bash setup_wsl.sh
```

脚本会执行需要 sudo 的步骤：安装 MySQL、开发头文件、Python venv，编译 `libfhope.so`，复制到 MySQL 插件目录，并创建默认数据库：

- 数据库：`test_db`
- 用户：`user`
- 密码：`123456`

这些值可用环境变量覆盖：

```bash
export FH_DB_NAME=test_db
export FH_DB_USER=user
export FH_DB_PASSWORD=123456
export PIP_INDEX_URL=https://pypi.tuna.tsinghua.edu.cn/simple
```

## 手动步骤

如果不使用一键脚本，可以分步执行：

```bash
sudo apt update
sudo apt install -y mysql-server default-libmysqlclient-dev build-essential python3 python3-pip python3-venv
sudo service mysql start

python3 -m venv .venv
. .venv/bin/activate
python -m pip install -r requirements.txt

bash build.sh --install
sudo mysql
```

在 MySQL root 会话中执行：

```sql
CREATE DATABASE IF NOT EXISTS test_db;
CREATE USER IF NOT EXISTS 'user'@'localhost' IDENTIFIED BY '123456';
GRANT ALL PRIVILEGES ON test_db.* TO 'user'@'localhost';
FLUSH PRIVILEGES;
```

然后导入 SQL：

```bash
mysql -uuser -p123456 test_db < init.sql
```

## 运行实验

原始指导书示例：

```bash
. .venv/bin/activate
python client.py demo
```

重复插入同一明文，观察重编码和叶子分裂：

```bash
python client.py repeat --value apple --count 140 --policy left
```

`--policy left` 会把重复值持续插入到相同值域的左侧，使同一个编码间隔不断被压缩，更容易触发 `Recode`。默认 `M=128`，插入到第 128 条附近可观察到叶子节点分裂，输出中的 `leaves` 会从 `1` 变为 `2`，`event` 会显示 `split`。

常用参数：

```bash
python client.py repeat --count 200 --policy left --every 5 --show-changes 5
python client.py repeat --count 140 --policy random
python client.py repeat --count 140 --policy right
```

## 无 MySQL 预验证

如果 MySQL 还没有安装，可以先运行同一份 `Node.cpp` 的本地模拟程序，确认编码树逻辑能复现重编码和分裂：

```bash
bash simulate_repeat.sh 140 10
```

这不会测试 UDF、存储过程或 Python 加密，只验证 FH-OPE 编码树本身。真实实验仍以 `client.py repeat` 连接 MySQL 的结果为准。

输出列含义：

- `returned`：`FHInsert` 返回的编码；为 `0` 表示本次触发了重编码。
- `changed`：本次插入后已有密文编码发生变化的数量。
- `update_range`：服务端需要同步更新的编码区间。
- `height` / `leaves` / `max_leaf`：调试用 UDF 返回的编码树状态。
- `event`：`recode` 表示编码更新，`split` 表示编码树叶子分裂。
