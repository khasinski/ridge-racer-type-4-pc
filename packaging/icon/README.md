# Application icon

`ridge-racer-type-4.png` (1080x1080) is the icon contributed by
[amame-a](https://github.com/amame-a) in
[issue #2](https://github.com/khasinski/ridge-racer-type-4-pc/issues/2):
black R4 and pc-port lettering on yellow. The submitted PNG is kept intact.
The lettering uses Orbitron, Copyright 2018 The Orbitron Project Authors,
under the SIL Open Font License 1.1 (see `OFL-Orbitron.txt`).

`build.sh` renders it into the derived files:

| File | Use |
| --- | --- |
| `packaging/macos/RidgeRacerType4.icns` | macOS bundle icon, 16 to 512 pt at 1x and 2x (`iconutil`) |
| `packaging/windows/ridge-racer-type-4.ico` | Windows executable icon, 16 to 256 px (`ridge-racer-type-4.rc.in`) |
| `packaging/linux/ridge-racer-type-4.png` | Linux AppImage icon, 1024 px |

Run `packaging/icon/build.sh` after replacing the PNG; it needs `sips` and
`iconutil` (macOS) and ImageMagick's `magick`.
