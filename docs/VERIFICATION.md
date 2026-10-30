# Local verification — 2026-10-30

Scope: only KetImageViewer and its gallery/installed-package consumer. No KétChat, KetSuite, server or other deployment suites were run.

Environment: macOS 26.2, Apple Silicon arm64, Qt 6.11.2, CMake/Ninja and Apple Clang.

## Build and contract tests

```sh
cmake --preset dev
cmake --build --preset dev -j8
ctest --test-dir build/dev -R '^image_viewer_contract$' --output-on-failure
```

Passed: one CTest suite, 11 behavior test methods (13 Qt Test passes including init/cleanup), zero failures. The contract suite runs with Qt's offscreen platform. It exercises:

- Fit/zoom anchor, pan clamping, rotation, resize and small-image/invalid zoom behavior.
- Synthesized wheel, native pinch, keyboard and double-click events.
- Atomic collection validation, navigation boundaries and empty state.
- Actual PNG byte/file decoding, encoded-byte/pixel limits and invalid inputs.
- Provider cancellation, one-use/stale tokens, failure/retry and preloading neighborhood.
- Rotation, clipboard output, PNG alpha/dimensions and atomic export failure.
- Cache eviction with visible-image pinning, clear/destruction during worker decode and reentrant completion.

## Installed package

Static Debug and shared Release builds were each installed and consumed by the separate `tests/consumer` project via `find_package(KetImageViewer CONFIG REQUIRED)` and `Ket::ImageViewer`. Both executables started and returned zero using `-platform offscreen`.

```sh
cmake --install build/dev --prefix "$PWD/stage"
cmake -S tests/consumer -B build/consumer -G Ninja -DCMAKE_PREFIX_PATH="$PWD/stage"
cmake --build build/consumer
./build/consumer/consumer -platform offscreen

cmake -S . -B build/shared -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=ON -DKET_IMAGE_VIEWER_BUILD_EXAMPLES=OFF -DKET_IMAGE_VIEWER_BUILD_TESTS=OFF
cmake --build build/shared -j8
cmake --install build/shared --prefix "$PWD/build/stage-shared"
cmake -S tests/consumer -B build/consumer-shared -G Ninja -DCMAKE_PREFIX_PATH="$PWD/build/stage-shared"
cmake --build build/consumer-shared
./build/consumer-shared/consumer -platform offscreen
```

CMake's optional Vulkan-header lookup was unavailable; it did not block this QWidget/QPainter build. Offscreen consumer runs reported a platform font-alias warning. The demo uses the system font and has no such warning.

## Native rendering and launch

The Cocoa demo rendered screenshots with `--capture` at 1120 × 800 logical pixels in light/dark mode and 480 × 800 in light mode. Visual inspection verified readable toolbar colors, image fit, thumbnail strip and narrow-width toolbar overflow. A fixed-size transparent thumbnail frame corrected title alignment for unloaded/portrait items. The final narrow render was inspected again after that correction.

The newly built `.app` was opened with Launch Services; a running process was verified. Desktop automation could not attach to its window (`cgWindowNotFound`), so native pointer-driven interaction and accessibility-tree inspection were not verified. Input behavior is covered by synthesized Qt events, not by a real trackpad/human session.

## Not verified / not delivered

Windows/Linux execution; Qt 6.8 specifically (local Qt is 6.11.2); real-device gestures; screen-reader behavior; EXIF orientation with a dedicated fixture; every optional codec; large-scale performance. Animated images use their first frame only.

This is a local library/demo build and installable CMake package. It is not a KétChat integration, public remote/release, standalone Qt-runtime bundle, signed or notarized installer.
