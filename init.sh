#!/usr/bin/env bash

set -euo pipefail

git submodule update --init
(cd contiki-uwb && git submodule update --init)
