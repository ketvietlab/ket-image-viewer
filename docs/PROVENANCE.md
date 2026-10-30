# Provenance and dependencies

The implementation in this repository was written independently for Két. No Telegram Desktop, qView, nomacs or other image-viewer source files, assets or patches are vendored here. They were considered as product/interaction references only. Earlier Telegram-copy experiments in other repositories are not dependencies of this project.

The transform engine uses a continuous scale, QTransform mapping and bounds clamping. The gallery/loading layer and generated demo artwork are original code in this repository. There are no downloaded sample photos, bundled icons or bundled font files.

External build/runtime dependencies are Qt 6 Core/Gui/Widgets/Concurrent and a C++17 standard library; focused tests additionally use Qt Test. Qt remains separately licensed. Optional image plugins and their codecs remain the host application's dependency and licensing responsibility. This repository's MIT license does not relicense Qt, fonts or images supplied by a host.

Két Sans is not redistributed here. A local demo may load an authorized local font through `--font`. Consumers may use their own font/palette or Két design tokens.
