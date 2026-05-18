#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")"

if ! command -v mysql_config >/dev/null 2>&1; then
    echo "mysql_config not found. Install MySQL development files first:"
    echo "  sudo apt update"
    echo "  sudo apt install -y mysql-server default-libmysqlclient-dev build-essential"
    exit 1
fi

g++ -std=c++17 -Wall -Wextra -Wno-unused-parameter -shared -fPIC \
    Node.cpp UDF.cpp $(mysql_config --include) \
    -o libfhope.so

echo "built: $(pwd)/libfhope.so"

if [[ "${1:-}" == "--install" ]]; then
    plugin_dir="$(mysql_config --plugindir)"
    sudo cp libfhope.so "$plugin_dir/"
    echo "installed: $plugin_dir/libfhope.so"
fi
