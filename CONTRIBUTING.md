# Contributing

Keep the library host-independent: no app-specific network APIs, credentials, account state or global QApplication style changes. Public interfaces live under `include/ket/imageviewer`; implementation details stay under `src`.

Use C++17, Qt 6.8+, four-space indentation and the supplied clang-format configuration. Describe API/behavior changes and update the integration documentation. Add focused tests for geometry, source lifecycle, cancellation, limits and export semantics when those contracts change. Pure styling fixes need representative rendered verification, not implementation-mirroring tests.

Run the build and the scoped `image_viewer_contract` suite described in the README. For public-header or CMake export changes, install to a temporary prefix and build `tests/consumer` with `CMAKE_PREFIX_PATH` pointing at it. Check both static/shared linkage when changing symbol visibility or dependency exports. Report OS, Qt version, commands and unverified platforms; a macOS pass does not establish Windows/Linux support.

Do not import upstream viewer code or assets without checking and recording their provenance and license compatibility. Contributions are under this repository's MIT license. Do not commit generated builds, private images, access tokens or local font files.
