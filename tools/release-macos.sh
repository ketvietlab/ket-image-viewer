#!/bin/bash
# Build a developer SDK. Qt is supplied separately by the consuming application.
set -euo pipefail
cd "$(dirname "$0")/.."
version="0.1.0"
architecture="$(uname -m)"
qt_version="$(qmake -query QT_VERSION)"
revision="$(git rev-parse HEAD)"
if [[ -n "$(git status --porcelain)" ]]; then
  echo "Commit source changes before packaging a reproducible release." >&2
  exit 1
fi
sdk_name="ket-image-viewer-${version}-macos-${architecture}-qt${qt_version}"
out="$PWD/dist/v${version}"
mkdir -p "$out"
cmake -S . -B build/release -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_SHARED_LIBS=OFF -DKET_IMAGE_VIEWER_BUILD_EXAMPLES=ON -DKET_IMAGE_VIEWER_BUILD_TESTS=ON
cmake --build build/release --parallel 8
ctest --test-dir build/release -R '^image_viewer_contract$' --output-on-failure
cmake --install build/release --prefix "$out/$sdk_name"
cmake -S tests/consumer -B build/release-consumer -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="$out/$sdk_name"
cmake --build build/release-consumer
./build/release-consumer/consumer -platform offscreen
printf 'Version: %s\nCommit: %s\nPlatform: macOS %s\nArchitecture: %s\nQt: %s\nLinkage: static\nConfiguration: Release\nQt runtime not included.\n' \
  "$version" "$revision" "$(sw_vers -productVersion)" "$architecture" "$qt_version" > "$out/$sdk_name/BUILD-INFO.txt"
git archive --format=tar.gz --prefix="ket-image-viewer-${version}/" \
  --output="$out/ket-image-viewer-${version}-source.tar.gz" "$revision"
COPYFILE_DISABLE=1 tar -czf "$out/$sdk_name.tar.gz" -C "$out" "$sdk_name"
(cd "$out" && shasum -a 256 ./*.tar.gz > SHA256SUMS)
printf 'Artifacts: %s\n' "$out"
