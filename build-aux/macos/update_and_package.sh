#!/usr/bin/env bash
#
# build-aux/macos/update_and_package.sh
#
# Convenience wrapper for LOCAL use: pulls the latest source, then packages.
#
# CI must NEVER call this script -- actions/checkout already pins the
# checkout to the exact commit that triggered the run; a `git pull` here
# would silently build whatever landed on master after that point instead,
# breaking reproducibility. CI calls package_dmg.sh directly.
#
# Usage: same arguments as package_dmg.sh, passed straight through.
#   ./update_and_package.sh --sign

set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")/../.."
git submodule update --init --recursive
git pull
exec "$(dirname "${BASH_SOURCE[0]}")/package_dmg.sh" "$@"
