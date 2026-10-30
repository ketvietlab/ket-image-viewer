# Két Image Viewer

An independently written, embeddable image viewer for Qt 6 Widgets applications. Version 0.1.0 provides a reusable canvas and gallery widget without coupling to KétChat, a backend, authentication, or a window manager.

**License:** MIT for this repository. Qt and its image plugins retain their own licenses.

## Features

- Cursor-anchored continuous zoom, constrained drag panning, pinch gestures and fit/100% modes.
- Clockwise rotation, keyboard navigation, thumbnails and transparent-image checkerboard.
- Local files, encoded bytes, `QImage`, or asynchronous host-provided images.
- Worker-thread file reads/decoding, bounded decoded-image cache and adjacent-image preloading.
- One-use request tokens: old replies cannot replace a new collection.
- Empty/loading/error states, retry, clipboard copy and atomic export of the rotated image.
- Host palette/font/style inheritance; optional built-in toolbar and thumbnail strip.
- CMake target `Ket::ImageViewer`, static/shared builds, installable package, native demo and focused tests.

## Build and try

Requires CMake 3.24+, a C++17 compiler, Qt 6.8+ with Widgets/Concurrent (Test for tests), and Ninja for the preset. Set `CMAKE_PREFIX_PATH` to your Qt installation if it is not discoverable.

```sh
cmake --preset dev
cmake --build --preset dev
ctest --test-dir build/dev -R '^image_viewer_contract$' --output-on-failure
```

On macOS:

```sh
open 'build/dev/examples/gallery/Két Image Viewer.app'
```

On Linux/Windows, run the `Két Image Viewer` executable in `build/dev/examples/gallery` (with `.exe` on Windows). The demo starts with original generated samples, including an intentional missing-file case to try error/retry. Use **Open images…** or pass local file paths. It supports `--dark`, `--width 480`, `--capture /path/to/capture.png` and optional `--font /path/to/font.ttf`.

The demo owns a Fusion style and Két-colored light/dark palettes. The library does not change application-wide style, font or palette. Fonts are not bundled.

## Embed

```cmake
# Either include a checkout:
add_subdirectory(third_party/ket-image-viewer)
# Or use an installed package:
# find_package(KetImageViewer 0.1 CONFIG REQUIRED)
target_link_libraries(my_app PRIVATE Ket::ImageViewer)
```

```cpp
#include <ket/imageviewer/image_viewer.h>
using namespace ket::imageviewer;

auto *viewer = new ImageViewer(parent);
ImageItem photo;
photo.id = "attachment-42";   // Stable, unique within this collection.
photo.title = "Our workspace";
photo.filePath = "/absolute/path/photo.jpg";
viewer->setItems({photo});
```

Install a package with `cmake --install build/dev --prefix /your/prefix`. Configure with `-DBUILD_SHARED_LIBS=ON` for a shared library. Examples and tests default to OFF when this repository is added as a subdirectory; explicitly enable them with `KET_IMAGE_VIEWER_BUILD_EXAMPLES` / `KET_IMAGE_VIEWER_BUILD_TESTS`.

See [API and integration](docs/API.md), [provenance](docs/PROVENANCE.md) and [verification](docs/VERIFICATION.md).

## Controls

| Input | Result |
| --- | --- |
| Mouse wheel / native pinch | Zoom around the pointer |
| Trackpad pixel scroll | Pan; Ctrl + scroll zooms |
| Left drag | Pan within image edges |
| Double click | Toggle fit and 100% |
| ← / → | Previous / next image, no wrap |
| + / − | Zoom in / out |
| F / 0 / R | Fit / 100% / rotate clockwise |

Canvas shortcuts apply while the canvas has focus. Demo-only: Cmd/Ctrl+O opens files; Escape exits fullscreen or closes the demo.

## Version 0.1 boundaries

This release is a foundation for integration and evaluation, not an already integrated KétChat feature. It is Qt Widgets, not QML/Qt Quick. Supported formats depend on the host's Qt image plugins. Animated formats display only the first frame. There is no video, tiled huge-image engine, HDR/color-managed display pipeline, editing history or built-in network client.

100% means one source pixel per **Qt logical pixel**, not necessarily one physical display pixel on Retina. Fit never enlarges a small image. Switching images resets the viewport. Rotation is view-only until export; export re-encodes pixels and does not preserve original bytes or metadata.

The current verification was performed on macOS Apple Silicon. Windows/Linux builds, platform-specific gesture behavior and screen-reader interaction still need platform validation. Local app builds use the installed Qt runtime; no standalone signed/notarized distribution is included.
