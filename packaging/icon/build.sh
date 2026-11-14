#!/bin/sh
# Regenerate the application icons from the source PNG:
#   packaging/icon/ridge-racer-type-4.png        source artwork
#   packaging/macos/RidgeRacerType4.icns         macOS bundle icon (iconutil)
#   packaging/windows/ridge-racer-type-4.ico     Windows executable icon
# Requires sips and iconutil (macOS) and ImageMagick (magick).
set -eu
cd "$(dirname "$0")/../.."
src=packaging/icon/ridge-racer-type-4.png
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT

iconset="$tmp/RidgeRacerType4.iconset"
mkdir "$iconset"
for size in 16 32 128 256 512; do
    sips -z $size $size "$src" --out "$iconset/icon_${size}x${size}.png" > /dev/null
    sips -z $((size * 2)) $((size * 2)) "$src" --out "$iconset/icon_${size}x${size}@2x.png" > /dev/null
done
iconutil -c icns "$iconset" -o packaging/macos/RidgeRacerType4.icns

magick "$src" -define icon:auto-resize=256,128,64,48,32,24,16 \
    packaging/windows/ridge-racer-type-4.ico
magick "$src" -resize 1024x1024 packaging/linux/ridge-racer-type-4.png
