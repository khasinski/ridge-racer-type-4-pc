# Releasing

Releases are cut from `main` by pushing a version tag (`0.1`, `0.2`, ...).
Four workflows build the packages and attach them to the GitHub release with
the same name as the tag; a fifth sums them:

| Workflow | Runner | Package |
| --- | --- | --- |
| `release-macos.yml` | `macos-15` (arm64) | `Ridge-Racer-Type-4-<version>-macos-arm64.zip` (signed, notarized, stapled app) |
| `release-linux.yml` | `ubuntu-24.04`, `ubuntu-24.04-arm` | `Ridge-Racer-Type-4-<version>-linux-x86_64.tar.gz` and `.AppImage`, the same for `linux-arm64` |
| `release-windows.yml` | `windows-2022` (MSYS2 UCRT64) | `Ridge-Racer-Type-4-<version>-windows-x86_64-experimental.zip` |
| `release-checksums.yml` | after each of the above succeeds | `SHA256SUMS` over every attached package |
| `release-validation.yml` | all of the above | nothing published; build, tests and packaging only |

The release workflows configure with `-DRR4_DIAGNOSTICS=OFF`, so the
packages leave out the test and diagnostic hooks of `src/port/main.c`.

Every package contains `README.md`, `CHANGELOG.md`, `THIRD_PARTY_NOTICES.md`,
`LICENSE.md`, `source-commit.txt` and the license texts of everything
linked (`licenses/`, on macOS inside
`Ridge Racer Type 4.app/Contents/Resources/licenses/`, in the AppImage under
`usr/share/doc/ridge-racer-type-4/`). The list of license files is
`packaging/licenses.txt`; `packaging/stage.sh` copies them.
`source-commit.txt` names the commit (with `-dirty` when the tree had
changes), its `git describe` and the PsyZ commit it was built with. The
workflows run only the unit tests that need no game data (`ctest`,
headless) and never need a disc image.

The AppImage (`packaging/linux/appimage.sh`) holds the same executable as
the tarball, with SDL linked in statically; it loads the host's X11 or
Wayland, GL/Vulkan and audio libraries at run time, so it bundles no shared
libraries. appimagetool and the AppImage runtime are downloaded at pinned
versions and checked against their SHA-256.

## Cutting a release

1. Run **Release validation** on `main` (Actions → Release validation → Run
   workflow, or `gh workflow run release-validation.yml`) and wait for it to
   pass on macOS and Linux.
2. Update [CHANGELOG.md](../CHANGELOG.md): rename the top section to the
   version (`## 0.1`) and set `project(... VERSION 0.1.0)` in
   `CMakeLists.txt` (the macOS bundle and Windows executable versions come
   from it). Commit and push to `main`.
3. Tag and push the tag:

   ```sh
   git tag -a 0.1 -m "Ridge Racer Type 4 0.1-alpha"
   git push origin 0.1
   ```

4. Right away, create the draft release the workflows attach to (they wait
   up to 30 minutes for it):

   ```sh
   gh release create 0.1 --verify-tag --draft \
       --title "Ridge Racer Type 4 0.1-alpha" --notes-file notes.md
   ```

   Write `notes.md` from [release-notes-template.md](release-notes-template.md).
5. Wait for the three platform workflows and **Release checksums**
   (`gh run list --limit 10`). Without the `MACOS_*` secrets the macOS
   workflow attaches nothing (see *Signing secrets*): build, notarize and
   upload the macOS zip on a Mac (*Local build, signing and notarization*).
   Check the draft has all packages and `SHA256SUMS`
   (`gh release view 0.1`), download the macOS zip and check it (see
   *Verifying* below).
6. Publish: `gh release edit 0.1 --draft=false` (add `--prerelease` while the
   port is alpha if wanted).

To rebuild one platform for an existing tag, run its workflow by hand **on
the tag**, so the tag's own workflow file and `packaging/` scripts build it
(`gh workflow run release-linux.yml --ref 0.1`); a manual run on any other
ref stops with an error unless it is a dry run. It replaces its packages
(`--clobber`) and the checksums workflow follows; checksums ignore failed,
cancelled and dry runs. Run
`gh workflow run release-checksums.yml -f version=0.1` to re-sum by hand.
Tags made before this rule (0.1) carry the old workflows: their macOS
workflow still uploads an ad-hoc signed zip over the notarized one, so do
not re-run it for them.

A dry run builds, tests and packages any ref and attaches nothing, e.g. to
try a packaging change on `main` before a release:

```sh
gh workflow run release-linux.yml --ref main -f dry_run=true -f version=dryrun
```

The packages are kept as workflow artifacts.

## Signing secrets (macOS)

Without these secrets the macOS app is signed ad-hoc and not notarized; the
workflow warns and attaches nothing, so a re-run never replaces a zip that
was notarized on a Mac. The repository has no `MACOS_*` secrets at the
moment: macOS packages are notarized locally (next section). To have CI
notarize, set them once per repository:

| Secret | Value |
| --- | --- |
| `MACOS_CERT_P12` | base64 of the exported *Developer ID Application* certificate and private key (`.p12`) |
| `MACOS_CERT_PASSWORD` | the `.p12` export password |
| `MACOS_NOTARY_KEY` | base64 of the App Store Connect API key (`AuthKey_XXXXXXXXXX.p8`) |
| `MACOS_NOTARY_KEY_ID` | the API key ID |
| `MACOS_NOTARY_ISSUER_ID` (or `MACOS_ISSUER_ID`) | the API issuer ID |
| `MACOS_TEAM_ID` | the Apple team ID (`DRR99SUARM`) |

```sh
repo=khasinski/ridge-racer-type-4-pc
base64 -i DeveloperID.p12 | gh secret set MACOS_CERT_P12 --repo $repo
gh secret set MACOS_CERT_PASSWORD --repo $repo          # prompts for the value
base64 -i AuthKey_XXXXXXXXXX.p8 | gh secret set MACOS_NOTARY_KEY --repo $repo
gh secret set MACOS_NOTARY_KEY_ID --repo $repo --body XXXXXXXXXX
gh secret set MACOS_NOTARY_ISSUER_ID --repo $repo --body <issuer-uuid>
gh secret set MACOS_TEAM_ID --repo $repo --body DRR99SUARM
```

Export the certificate from Keychain Access (*My Certificates* → the
Developer ID Application identity → Export, `.p12`). Create the API key in
App Store Connect → Users and Access → Integrations → App Store Connect API
(role *Developer*). The same secrets as in the sibling ports can be reused.

## Local build, signing and notarization (fallback)

When CI signing is unavailable, build and notarize on a Mac with the
Developer ID identity in the login keychain and a notarytool keychain profile
(`xcrun notarytool store-credentials aya-notarize ...`, done once):

```sh
version=0.1
cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release -DRR4_MACOS_BUNDLE=ON -DBUILD_TESTING=ON \
    -DRR4_DIAGNOSTICS=OFF
cmake --build build-release --parallel
ctest --test-dir build-release --output-on-failure
app="build-release/Ridge Racer Type 4.app"
codesign --force --options runtime --timestamp \
    --sign "Developer ID Application: Krzysztof Hasinski (DRR99SUARM)" "$app"
codesign --verify --deep --strict --verbose=2 "$app"
ditto -c -k --sequesterRsrc --keepParent "$app" /tmp/rr4-notarize.zip
xcrun notarytool submit /tmp/rr4-notarize.zip --keychain-profile aya-notarize --wait
xcrun stapler staple "$app"
xcrun stapler validate "$app"

package=Ridge-Racer-Type-4-$version-macos-arm64
mkdir -p "dist/$package"
ditto "$app" "dist/$package/Ridge Racer Type 4.app"
packaging/stage.sh "dist/$package" build-release --no-licenses
ditto -c -k --sequesterRsrc --keepParent "dist/$package" "dist/$package.zip"
gh release upload $version "dist/$package.zip" --clobber
gh workflow run release-checksums.yml -f version=$version
```

If notarization is rejected, `xcrun notarytool log <submission-id>
--keychain-profile aya-notarize` explains why. The app needs no
entitlements: hardened runtime alone is enough.

## Verifying

```sh
ditto -x -k Ridge-Racer-Type-4-0.1-macos-arm64.zip /tmp/rr4-check
app="/tmp/rr4-check/Ridge-Racer-Type-4-0.1-macos-arm64/Ridge Racer Type 4.app"
codesign --verify --deep --strict --verbose=2 "$app"
spctl -a -vv "$app"          # accepted, source=Notarized Developer ID
xcrun stapler validate "$app"
shasum -a 256 -c SHA256SUMS --ignore-missing
```

A silent, windowless smoke run of the packaged game with your disc image:

```sh
PSYZ_HEADLESS=1 SDL_AUDIODRIVER=dummy RR4_FRAME_LIMIT=300 \
    "$app/Contents/MacOS/Ridge Racer Type 4" "/path/to/Ridge Racer Type 4 (Europe).cue"
```

## Notes

- The Windows build is experimental (MSYS2 UCRT64 GCC, LLP64). The
  executable links the GCC runtime statically and depends only on Windows
  system DLLs and the UCRT; the workflows check its imports and icon
  resource. It is built and tested (settings test, startup without a disc)
  but not yet played in CI; say so in the notes.
- Linux builds use the runner's default compiler (GCC). Clang works as well
  (the macOS builds use it).
- Never attach game data, disc images, memory cards or SDK files.
