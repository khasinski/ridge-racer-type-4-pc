#!/bin/sh
# Copy the documentation, notices and license texts that every release
# package carries into a package directory:
#   packaging/stage.sh <package-dir> <build-dir> [--no-licenses]
# <package-dir>/  README.md CHANGELOG.md THIRD_PARTY_NOTICES.md [LICENSE.md]
#                 source-commit.txt (see below)
#                 licenses/<source path of each packaging/licenses.txt entry>
#                 licenses/libwebp/{COPYING,PATENTS}
# source-commit.txt: the first line is the full commit, with "-dirty" when
# the work tree (or the PsyZ submodule) differs from it; then "describe:"
# (git describe --tags --always --dirty) and "psyz:" (the commit checked
# out in third_party/psyz, "-dirty" when it has changes, "(not the pinned
# <commit>)" when it is not the commit the superproject records).
# The macOS app carries the license texts in Contents/Resources/licenses
# already (CMake RR4_MACOS_BUNDLE), so its package passes --no-licenses.
set -eu
root=$(cd "$(dirname "$0")/.." && pwd)
dest=$1
build=$2
licenses=1
[ "${3:-}" = "--no-licenses" ] && licenses=0

mkdir -p "$dest"
for doc in README.md CHANGELOG.md THIRD_PARTY_NOTICES.md LICENSE.md; do
    if [ -f "$root/$doc" ]; then
        cp "$root/$doc" "$dest/"
    elif [ "$doc" != LICENSE.md ]; then
        echo "stage.sh: missing $doc" >&2
        exit 1
    fi
done
# git describe refreshes the index first, so a touched but unchanged file
# does not count as a change.
describe=$(git -C "$root" describe --tags --always --dirty)
dirty=
case $describe in *-dirty) dirty=-dirty ;; esac
psyz=$(git -C "$root/third_party/psyz" rev-parse HEAD 2>/dev/null || echo missing)
if [ "$psyz" != missing ]; then
    case $(git -C "$root/third_party/psyz" describe --always --dirty) in
        *-dirty) psyz=$psyz-dirty ;;
    esac
    pinned=$(git -C "$root" rev-parse HEAD:third_party/psyz 2>/dev/null || true)
    [ -z "$pinned" ] || [ "${psyz%-dirty}" = "$pinned" ] || psyz="$psyz (not the pinned $pinned)"
fi
{
    echo "$(git -C "$root" rev-parse HEAD)$dirty"
    echo "describe: $describe"
    echo "psyz: $psyz"
} > "$dest/source-commit.txt"

[ "$licenses" -eq 1 ] || exit 0
grep -v '^#' "$root/packaging/licenses.txt" | while read -r notice; do
    [ -n "$notice" ] || continue
    mkdir -p "$dest/licenses/$(dirname "$notice")"
    cp "$root/$notice" "$dest/licenses/$notice"
done
webp="$build/_deps/psyz_webp-src"
if [ ! -f "$webp/COPYING" ]; then
    echo "stage.sh: libwebp license not found in $webp" >&2
    exit 1
fi
mkdir -p "$dest/licenses/libwebp"
cp "$webp/COPYING" "$webp/PATENTS" "$dest/licenses/libwebp/"
