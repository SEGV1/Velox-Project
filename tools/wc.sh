#!/usr/bin/env sh
# Count lines across all first-party C/C++/header source files,
# excluding generated protobuf code under src/rpc/.

SCRIPT_DIR=$( cd "$(dirname "$0")" && pwd )
cd "${SCRIPT_DIR}/.."

find src/ \( -name "*.h" -o -name "*.cpp" -o -name "*.c" \) \
     -not -path "src/rpc/*" \
  | sort \
  | xargs wc -l
