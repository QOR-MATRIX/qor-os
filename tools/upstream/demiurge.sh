#!/usr/bin/env bash
# SPDX-License-Identifier: LicenseRef-QOR-OS-Proprietary
#
# Keeps what QOR OS still shares with demiurge-chain in step with upstream
# (QOR-0015, docs/UPSTREAM.md): ARQADE's run fixtures, carried at the same path
# here as there; its rule modules, kept identical; and Demiurge's one look,
# copied. The products themselves are developed here and no longer pulled.
#
#   tools/upstream/demiurge.sh status          last upstream commits, whether the rules match, and the core's pin
#   tools/upstream/demiurge.sh pull [REF]      pull the fixtures and the look from demiurge-chain REF (default: main)
#   tools/upstream/demiurge.sh patch COMMIT    write COMMIT's changes to all three as patches for upstream
#
# Pulling splits the fixtures' history in a separate clone of demiurge-chain
# (QOR_UPSTREAM_DIR, default ../_upstream/demiurge-chain beside this checkout),
# then merges it here with `git subtree pull`, in one merge commit. The clone
# is created on first use. Run from a clean working tree.

set -euo pipefail

repo=$(git -C "$(dirname "$0")/../.." rev-parse --show-toplevel)
upstream_url=${QOR_UPSTREAM_URL:-https://github.com/QOR-MATRIX/demiurge-chain.git}
upstream=${QOR_UPSTREAM_DIR:-$(dirname "$repo")/_upstream/demiurge-chain}

# Carried, identical here and upstream: ARQADE's games here and its server
# there replay the same runs (QOR-0015 decision 2).
carried=(products/arqade/tests/fixtures)
# Copied rather than carried: Demiurge's one look, which the products' tests read.
copied=(docs/design/tokens.json)
# ARQADE's rules, in the games but shared with its server: kept identical, and
# changed in both places at once (QOR-0015 decision 2).
rules=(
    products/arqade/games/orbital/logic/rules.mjs
    products/arqade/games/void-runner/logic/rules.mjs
    products/arqade/games/synapse/logic/rules.mjs
    products/arqade/games/rift-survivor/logic/rules.mjs
    products/arqade/games/arenas/logic/arena.mjs
)
# Developed here since QOR-0015; status says where each left upstream.
developed=(
    products/qq
    products/arqade/qt
    products/arqade/games
    tools/qor-media
)

usage() { sed -n '4,16p' "$0" | sed 's/^# \{0,1\}//'; exit 2; }

split_of() {
    git -C "$repo" log -1 --format='%(trailers:key=git-subtree-split,valueonly)' --grep="git-subtree-dir: $1\$" | tr -d '[:space:]'
}

clone() {
    if [ ! -d "$upstream/.git" ]; then
        mkdir -p "$(dirname "$upstream")"
        git clone --quiet "$upstream_url" "$upstream"
    fi
    git -C "$upstream" fetch --quiet origin
}

# Each rule module that is not the same here as at upstream COMMIT.
rules_differing() {
    local path
    for path in "${rules[@]}"; do
        [ "$(git -C "$repo" hash-object "$repo/$path")" = "$(git -C "$upstream" rev-parse --quiet --verify "$1:$path")" ] \
            || echo "$path"
    done
}

status() {
    local last differing
    for path in "${carried[@]}"; do
        last=$(split_of "$path")
        printf '%-32s split %s\n' "$path" "${last:-(none)}"
    done
    for path in "${developed[@]}"; do
        printf '%-32s developed here since QOR-0015, last split %s\n' "$path" "$(split_of "$path")"
    done
    if [ -d "$upstream/.git" ]; then
        differing=$(rules_differing origin/main)
        if [ -z "$differing" ]; then
            printf '%-32s %s\n' "ARQADE's rules" "the same as upstream main (as last fetched)"
        else
            printf '%-32s %s\n' "ARQADE's rules" "differ from upstream main (as last fetched):"
            while read -r path; do printf '  %s\n' "$path"; done <<< "$differing"
        fi
    fi
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
    local differing
    differing=$(rules_differing "$commit")
    if [ -n "$differing" ]; then
        echo "ARQADE's rules differ from demiurge-chain $short; change both at once (docs/UPSTREAM.md):" >&2
        while read -r path; do printf '  %s\n' "$path"; done <<< "$differing" >&2
    fi
    echo "Pulled demiurge-chain $short. Build and test the products (tools/dev/products.ps1) before pushing."
}

patch() {
    local commit=${1:?a commit}
    local out="$repo/build/upstream-patches"
    mkdir -p "$out"
    # The same paths upstream, so the patches apply there with `git am`.
    git -C "$repo" format-patch --quiet -1 "$commit" -o "$out" -- "${carried[@]}" "${copied[@]}" "${rules[@]}"
    echo "Patches in $out. Apply them on a branch of demiurge-chain with: git am $out/*.patch"
}

case ${1:-} in
    status) status ;;
    pull) shift; pull "$@" ;;
    patch) shift; patch "$@" ;;
    *) usage ;;
esac
