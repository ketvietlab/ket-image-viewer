# API and integration contract

## Layers

- `Viewport`: display-independent geometry for scale, pan, quarter-turn rotation and source-to-view mapping.
- `ImageCanvas`: synchronous single-image QWidget rendering and input. Use this for custom host controls.
- `ImageViewer`: gallery selection, source loading, cache, status, built-in toolbar and thumbnails.

All public widget calls, including `provideImage`, `provideData`, and `failRequest`, must run on the GUI thread. Deliver worker/network completions through a QObject context or queued invocation. Do not delete the canvas returned by `canvas()` or replace its content behind the gallery's back; use `ImageCanvas` separately if the host owns the image lifecycle.

## Collection

`setItems(items, index)` returns false and emits `collectionRejected` for duplicate/empty IDs, multiple local sources, or an invalid initial index. It leaves the existing collection unchanged. Empty collections require index 0. An accepted collection invalidates pending request tokens and clears the cache. IDs are opaque and must be unique within the collection.

Each `ImageItem` contains an ID, title, optional thumbnail and at most one source:

| Source | Handling |
| --- | --- |
| `filePath` | File read and image decode on a worker |
| `encodedData` | Decode on a worker |
| `image` | Already decoded QImage; validated against pixel limit |
| None | Emit `imageRequested(token, id)` for the host |

An empty byte array is treated as no local source. Use `failRequest` for a provider error, not an empty payload. `QImageReader` applies supported orientation metadata on decode. Decoding and displayed-format support depend on Qt plugins.

## Host-provided images

Connect before calling `setItems`. Keep downloads keyed by request token, not only image ID. The same ID may be requested again after cache eviction or retry.

```cpp
QObject::connect(viewer, &ImageViewer::imageRequested, host,
    [host](quint64 token, const QString &id) {
        host->fetchAttachment(token, id); // Your async adapter; not a library API.
    });
QObject::connect(viewer, &ImageViewer::requestCancelled, host,
    [host](quint64 token) { host->cancelAttachment(token); });
// In the adapter's GUI-thread completion, while viewer is still alive:
viewer->provideData(token, downloadedBytes);
// Or viewer->provideImage(token, decodedImage);
// Or viewer->failRequest(token, translatedError);
```

The host owns network requests, credentials, storage, timeouts, retry policy and download limits. Use a receiver context/QPointer for callbacks so a destroyed viewer is never dereferenced. Tokens are scoped to a viewer instance and accepted once; cancelled/unknown/repeated completions are ignored.

Navigation cancels provider requests outside the current preload neighborhood. `clear()`, replacement collections and load-limit changes invalidate all outstanding tokens. Destruction emits cancellation for outstanding provider tokens; cancellation handlers should only stop host work and must not call back into the destructing widget.

Loading/state/current/image-result notifications are posted to the GUI event loop to avoid reentrant mutation inside internal updates. Notifications from a replaced collection are suppressed. Within a collection, signals describe transitions; query `state()` and `currentId()` for current values if navigation happened before delivery. Host action signals (`saveRequested`, `closeRequested`) and collection rejection are immediate. Loading has no built-in timeout.

## Memory and concurrency

Defaults: 64 MiB encoded input, 24 × 1024 × 1024 decoded pixels per image, 128 MiB decoded cache, one adjacent image on either side, at most two active decode jobs per viewer. `setLoadLimits` validates configuration, clears loads/cache and reloads the current item. Encoded-byte and pixel bounds are checked before decoding and pixel dimensions again afterward. These limits are resource controls, not a sandbox for image codecs.

The cache is bounded, but total process memory is not bounded by `cacheKiB`: current displayed image, retained caller-supplied images/bytes/thumbnails, decoder buffers and running jobs consume additional memory. Prefer file paths or provider IDs for large collections. The visible image remains pinned even when larger than the cache. The thumbnail list is intended for ordinary galleries, not millions of items.

Obsolete queued jobs are dropped. Already-running Qt image decoders finish without publishing to a replaced collection. Destruction does not synchronously wait for decoders; a small bounded amount of work can continue. Jobs retain only their input and limits, never the widget.

## Display and host UI

`setToolbarVisible(false)` and `setThumbnailsVisible(false)` let a host supply its own chrome. `ImageCanvas` uses the host's Base, AlternateBase, Text and Highlight palette roles. The library makes no application-wide font/style/theme changes. Qt strings use `tr()`; translations are host-provided. Thumbnail titles are elided with full tooltip text, while the caption wraps. QToolBar uses its overflow menu at narrow widths.

Zoom is continuous and bounded between min(1%, fit scale) and 3200%. Pan clamps to visible image edges and centers axes smaller than the viewport. 100% is defined in logical Qt pixels; source DPR tags are normalized to 1. Native pinch, wheel and keyboard paths use the same geometry model. A new image resets rotation, pan and zoom to fit.

`saveRequested(id)` asks the host to select a path. `saveCurrent(path, &error)` atomically writes the full decoded image with current rotation, not the cropped visible viewport. It returns false on unsupported suffix/write failure. `copyCurrent()` explicitly writes the same image to the clipboard. Neither operation edits the source. `closeRequested()` asks the host to close its viewer surface; the widget does not close its parent window itself.

Qt/plugin requirements and GUI-thread affinity are part of the API. API/ABI stability across 0.x minor releases is not promised; CMake package compatibility is limited to the same minor version.
