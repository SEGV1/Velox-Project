#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR=$( cd "$(dirname "${BASH_SOURCE[0]}")" && pwd )
BUILD_DIR="${SCRIPT_DIR}/../build.debug"

echo "[velox] Configuring build with AddressSanitizer in ${BUILD_DIR}"
mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"

cmake -DCMAKE_BUILD_TYPE=Debug \
      -DUSE_ADDRESS_SANITIZER=ON \
      -DUSE_THREAD_SANITIZER=OFF \
      ..

echo "[velox] Building..."
make -j"$(nproc)"
