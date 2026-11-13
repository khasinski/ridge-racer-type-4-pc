#!/bin/sh
# Build the Linux AppImage from a staged package directory:
#   packaging/linux/appimage.sh <package-dir> <arch> <output.AppImage>
# <package-dir> is the tarball's directory (ridge-racer-type-4-pc, the icon,
# and what packaging/stage.sh copies). <arch> is x86_64 or arm64. SDL is
# linked into the executable statically, as in the tarball; it loads the
# X11/Wayland, GL/Vulkan and audio libraries of the host at run time, so the
# AppImage carries no shared libraries.
#
# appimagetool and the AppImage runtime are downloaded at pinned versions and
# checked against their SHA-256 (APPIMAGETOOL=<path> uses a local
# appimagetool instead; the runtime is still the pinned one).
set -eu
root=$(cd "$(dirname "$0")/../.." && pwd)
package=$(cd "$1" && pwd)
arch=$2
out=$3

tool_version=1.9.1
runtime_version=20251108
case $arch in
x86_64)
    tool_arch=x86_64
    tool_sha=ed4ce84f0d9caff66f50bcca6ff6f35aae54ce8135408b3fa33abfc3cb384eb0
    runtime_sha=2fca8b443c92510f1483a883f60061ad09b46b978b2631c807cd873a47ec260d
    ;;
arm64)
    tool_arch=aarch64
    tool_sha=f0837e7448a0c1e4e650a93bb3e85802546e60654ef287576f46c71c126a9158
    runtime_sha=00cbdfcf917cc6c0ff6d3347d59e0ca1f7f45a6df1a428a0d6d8a78664d87444
    ;;
*)
    echo "appimage.sh: unknown arch $arch (x86_64 or arm64)" >&2
    exit 1
    ;;
esac

work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT

# fetch <url> <sha256> <file>
fetch() {
    curl -fsSL --retry 3 -o "$3" "$1"
    echo "$2  $3" | sha256sum -c - > /dev/null || {
        echo "appimage.sh: checksum mismatch for $1" >&2
        exit 1
    }
}

runtime="$work/runtime-$tool_arch"
fetch "https://github.com/AppImage/type2-runtime/releases/download/$runtime_version/runtime-$tool_arch" \
    "$runtime_sha" "$runtime"
tool=${APPIMAGETOOL:-}
if [ -z "$tool" ]; then
    tool="$work/appimagetool"
    fetch "https://github.com/AppImage/appimagetool/releases/download/$tool_version/appimagetool-$tool_arch.AppImage" \
        "$tool_sha" "$tool"
    chmod +x "$tool"
fi

app="$work/Ridge Racer Type 4.AppDir"
doc="$app/usr/share/doc/ridge-racer-type-4"
mkdir -p "$app/usr/bin" "$app/usr/share/applications" \
    "$app/usr/share/icons/hicolor/1024x1024/apps" "$doc"
cp "$package/ridge-racer-type-4-pc" "$app/usr/bin/"
cp "$root/packaging/linux/ridge-racer-type-4.desktop" "$app/"
cp "$root/packaging/linux/ridge-racer-type-4.desktop" "$app/usr/share/applications/"
cp "$root/packaging/icon/ridge-racer-type-4.png" "$app/ridge-racer-type-4.png"
cp "$root/packaging/icon/ridge-racer-type-4.png" "$app/usr/share/icons/hicolor/1024x1024/apps/"
ln -s ridge-racer-type-4.png "$app/.DirIcon"
# Documentation, notices, licenses and source-commit.txt from stage.sh.
(cd "$package" && find . -type f ! -name ridge-racer-type-4-pc ! -name ridge-racer-type-4.png \
    ! -name runtime-dependencies.txt) | while read -r f; do
    mkdir -p "$doc/$(dirname "$f")"
    cp "$package/$f" "$doc/$f"
done
for f in README.md THIRD_PARTY_NOTICES.md source-commit.txt licenses/libwebp/COPYING; do
    test -f "$doc/$f" || { echo "appimage.sh: missing $f in $package" >&2; exit 1; }
done
# The memory cards (bu00, bu10) and the preferences go where the tarball
# puts them: the cards in the working directory the AppImage is started
# from, the preferences in the user's configuration directory.
cat > "$app/AppRun" <<'APPRUN'
#!/bin/sh
here=$(dirname "$(readlink -f "$0")")
exec "$here/usr/bin/ridge-racer-type-4-pc" "$@"
APPRUN
chmod +x "$app/AppRun" "$app/usr/bin/ridge-racer-type-4-pc"

# appimagetool is itself an AppImage: extract-and-run needs no FUSE.
ARCH=$tool_arch APPIMAGE_EXTRACT_AND_RUN=1 "$tool" --no-appstream \
    --runtime-file "$runtime" "$app" "$out"
chmod +x "$out"
