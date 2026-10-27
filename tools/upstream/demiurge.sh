#!/usr/bin/env bash
# SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
#
# Keeps the code QOR OS carries from demiurge-chain in step with upstream
# (QOR-0002, docs/UPSTREAM.md). The carried paths are the same here as there.
#
#   tools/upstream/demiurge.sh status          each carried path's last upstream commit, and the core's pin
#   tools/upstream/demiurge.sh pull [REF]      pull every carried path from demiurge-chain REF (default: main)
#   tools/upstream/demiurge.sh patch COMMIT    write COMMIT's changes to carried paths as patches for upstream
#
# Pulling splits each path's history in a separate clone of demiurge-chain
# (QOR_UPSTREAM_DIR, default ../_upstream/demiurge-chain beside this checkout),
# then merges it here with `git subtree pull`, one merge commit per path. The
# clone is created on first use. Run from a clean working tree.

set -euo pipefail

repo=$(git -C "$(dirname "$0")/../.." rev-parse --show-toplevel)
upstream_url=${QOR_UPSTREAM_URL:-https://github.com/QOR-MATRIX/demiurge-chain.git}
upstream=${QOR_UPSTREAM_DIR:-$(dirname "$repo")/_upstream/demiurge-chain}

# The carried paths, identical here and upstream.
carried=(
    products/qq
    products/arqade/qt
    products/arqade/games
    products/arqade/tests/fixtures
    tools/qor-media
)
# Copied rather than carried: Demiurge's one look, which the carried tests read.
copied=(docs/design/tokens.json)

usage() { sed -n '4,13p' "$0" | sed 's/^# \{0,1\}//'; exit 2; }

clone() {
    if [ ! -d "$upstream/.git" ]; then
        mkdir -p "$(dirname "$upstream")"
        git clone --quiet "$upstream_url" "$upstream"
    fi
    git -C "$upstream" fetch --quiet origin
}

status() {
    for path in "${carried[@]}"; do
        last=$(git -C "$repo" log -1 --format='%(trailers:key=git-subtree-split,valueonly)' --grep="git-subtree-dir: $path\$" | tr -d '[:space:]')
        printf '%-32s split %s\n' "$path" "${last:-(none)}"
    done
    printf '%-32s %s\n' "QOR core pin" "$(sed -n 's/^set(QOR_CORE_COMMIT "\([0-9a-f]*\)".*/\1/p' "$repo/core/CMakeLists.txt")"
}

pull() {
    local ref=${1:-main}
    if [ -n "$(git -C "$repo" status --porcelain)" ]; then
        echo "The working tree is not clean; commit or stash first." >&2
        exit 1
    fi
    clone
    local commit
    commit=$(git -C "$upstream" rev-parse "origin/$ref^{commit}" 2>/dev/null || git -C "$upstream" rev-parse "$ref^{commit}")
    local short=${commit:0:7}
    for path in "${carried[@]}"; do
        local branch="qor-split/${path//\//-}" split
        echo "== $path"
        # Deterministic: the same upstream history always splits to the same commits, so pulls are incremental.
        split=$(git -C "$upstream" subtree split --quiet --prefix="$path" "$commit")
        git -C "$upstream" branch --quiet -f "$branch" "$split"
        git -C "$repo" subtree pull --prefix="$path" "$upstream" "$branch" -m "Pull $path from demiurge-chain $short

git subtree pull of $path at demiurge-chain $commit (QOR-0002)."
    done
    for path in "${copied[@]}"; do
        git -C "$upstream" cat-file blob "$commit:$path" > "$repo/$path"
        git -C "$repo" add "$path"
    done
    if ! git -C "$repo" diff --cached --quiet; then
        git -C "$repo" commit --quiet -m "Copy ${copied[*]} from demiurge-chain $short"
    fi
    echo "Pulled demiurge-chain $short. Build and test the products (tools/dev/products.ps1) before pushing."
}

patch() {
    local commit=${1:?a commit}
    local out="$repo/build/upstream-patches"
    mkdir -p "$out"
    # The same paths upstream, so the patches apply there with `git am`.
    git -C "$repo" format-patch --quiet -1 "$commit" -o "$out" -- "${carried[@]}" "${copied[@]}"
    echo "Patches in $out. Apply them on a branch of demiurge-chain with: git am $out/*.patch"
}

case ${1:-} in
    status) status ;;
    pull) shift; pull "$@" ;;
    patch) shift; patch "$@" ;;
    *) usage ;;
esac
