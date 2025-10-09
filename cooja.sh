#!/usr/bin/env bash

set -euo pipefail

if [ "$#" -eq 0 ]; then
    echo "Usage: $0 <csc_file_path>"
    exit 1
fi

(cd cooja && ./gradlew run --args="../$1")
