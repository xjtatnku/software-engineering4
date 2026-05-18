#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")"

DB_NAME="${FH_DB_NAME:-test_db}"
DB_USER="${FH_DB_USER:-user}"
DB_PASSWORD="${FH_DB_PASSWORD:-123456}"

mysql -u"${DB_USER}" -p"${DB_PASSWORD}" "${DB_NAME}" < init.sql
