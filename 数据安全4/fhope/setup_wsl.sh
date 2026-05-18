#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")"

DB_NAME="${FH_DB_NAME:-test_db}"
DB_USER="${FH_DB_USER:-user}"
DB_PASSWORD="${FH_DB_PASSWORD:-123456}"
PIP_INDEX_URL="${PIP_INDEX_URL:-https://pypi.tuna.tsinghua.edu.cn/simple}"

sudo apt update
sudo apt install -y mysql-server default-libmysqlclient-dev build-essential python3 python3-pip python3-venv

python3 -m venv .venv
. .venv/bin/activate
python -m pip install -i "${PIP_INDEX_URL}" --upgrade pip setuptools wheel
python -m pip install -i "${PIP_INDEX_URL}" -r requirements.txt

sudo service mysql start || sudo systemctl start mysql

bash build.sh --install

sudo mysql <<SQL
CREATE DATABASE IF NOT EXISTS \`${DB_NAME}\`;
CREATE USER IF NOT EXISTS '${DB_USER}'@'localhost' IDENTIFIED BY '${DB_PASSWORD}';
CREATE USER IF NOT EXISTS '${DB_USER}'@'%' IDENTIFIED BY '${DB_PASSWORD}';
GRANT ALL PRIVILEGES ON \`${DB_NAME}\`.* TO '${DB_USER}'@'localhost';
GRANT ALL PRIVILEGES ON \`${DB_NAME}\`.* TO '${DB_USER}'@'%';
FLUSH PRIVILEGES;
SQL

mysql -u"${DB_USER}" -p"${DB_PASSWORD}" "${DB_NAME}" < init.sql

echo "FH-OPE environment is ready."
echo "Run:"
echo "  . .venv/bin/activate"
echo "  python client.py repeat --count 140 --policy left"
