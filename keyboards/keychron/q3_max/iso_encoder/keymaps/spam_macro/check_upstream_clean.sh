#!/usr/bin/env bash
# Fails when anything outside our own folders differs from Keychron's code, so upstream merges stay conflict-free.
# Usage: check_upstream_clean.sh [base-ref]   (default: upstream/2025q3)
set -euo pipefail
cd "$(git rev-parse --show-toplevel)"

base=${1:-upstream/2025q3}
ours='^(CLAUDE\.md|keychron-web/|tests/keychron_spam_macro/|keyboards/keychron/q3_max/iso_encoder/keymaps/spam_macro/|keyboards/keychron/q3_max/firmware/)'

# Committed, staged and unstaged changes. Submodule checkouts are ignored only in the working tree,
# where a stale checkout is not a change we made; committed or staged pin changes still count.
outside=$(
    {
        git diff --no-renames --name-only "$base"...HEAD
        git diff --no-renames --cached --name-only
        git diff --no-renames --name-only --ignore-submodules=all
    } | sort -u | grep -vE "$ours" || true
)

if [ -n "$outside" ]; then
    echo "Changed outside our folders (compared with $base):" >&2
    echo "$outside" >&2
    exit 1
fi
echo "OK: only our folders differ from $base"
