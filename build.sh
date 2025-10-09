#!/usr/bin/env bash

set -euo pipefail

if [ "$#" -eq 0 ]; then
    echo "Usage: $0 <project_folder>"
    exit 1
fi

docker run --rm --volume .:/repo --workdir /repo contiker/contiki-ng bash -c "cd '$1' && make"
