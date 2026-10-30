// SPDX-License-Identifier: MIT
#include "decoder.h"
#include <QAction>
#include <QApplication>
#include <QCache>
#include <QClipboard>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QHash>
#include <QImageWriter>
#include <QLabel>
#include <QListWidget>
#include <QPainter>
#include <QPointer>
#include <QQueue>
#include <QSaveFile>
#include <QSet>
#include <QSignalBlocker>
#include <QTimer>
#include <QToolBar>
#include <QVBoxLayout>
#include <QtConcurrent/QtConcurrentRun>
#include <algorithm>
#include <atomic>
#include <ket/imageviewer/image_viewer.h>

namespace ket::imageviewer {
namespace {
QIcon thumbnailIcon(const QImage &image) {
    QPixmap frame(72, 54);
    frame.fill(Qt::transparent);
    if (!image.isNull()) {
        const auto scaled = image.scaled(frame.size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
        QPainter painter(&frame);
        painter.drawImage(
            QPoint((frame.width() - scaled.width()) / 2, (frame.height() - scaled.height()) / 2), scaled);
    }
    return QIcon(frame);
}
} // namespace
class ImageViewer::Private {
  public:
    explicit Private(ImageViewer *owner) : q(owner), cache(limits.cacheKiB) {}
    struct Job {
        ImageItem item;
        quint64 revision;
        LoadLimits limits;
        std::shared_ptr<std::atomic_bool> cancelled;
    };
    ImageViewer *q;
    QList<ImageItem> items;
    int index = -1;
    State state = State::Empty;
    QString error;
    LoadLimits limits;
    quint64 revision = 0, sequence = 0;
    QCache<QString, QImage> cache;
    QHash<QString, QString> failures;
    QSet<QString> pending;
    QHash<quint64, QString> external;
    QQueue<Job> queue;
    int active = 0;
    std::shared_ptr<std::atomic_bool> cancelled = std::make_shared<std::atomic_bool>(false);
    ImageCanvas *canvas = nullptr;
    QToolBar *toolbar = nullptr;
    QLabel *caption = nullptr;
    QListWidget *thumbnails = nullptr;
    QAction *back = nullptr, *forward = nullptr, *fit = nullptr, *actual = nullptr, *zoomIn = nullptr,
            *zoomOut = nullptr, *rotate = nullptr, *copy = nullptr, *save = nullptr, *retry = nullptr;
    QLabel *zoomLabel = nullptr;
    QString displayedId;

    template <typename F> void notify(F fn) {
        const auto epoch = revision;
        QTimer::singleShot(0, q, [this, epoch, fn] {
            if (revision == epoch)
                fn();
        });
    }
    QString id() const { return index >= 0 ? items[index].id : QString(); }
    void reset() {
        ++revision;
        cancelled->store(true);
        cancelled = std::make_shared<std::atomic_bool>(false);
        const auto tokens = external.keys();
        external.clear();
        pending.clear();
        queue.clear();
        cache.clear();
        failures.clear();
        displayedId.clear();
        for (auto token : tokens)
            QTimer::singleShot(0, q, [owner = q, token] { emit owner->requestCancelled(token); });
    }
    void controls() {
        const bool ready = state == State::Ready;
        back->setEnabled(index > 0);
        forward->setEnabled(index >= 0 && index + 1 < items.size());
        for (auto *action : {fit, actual, zoomIn, zoomOut, rotate, copy, save})
            action->setEnabled(ready);
        retry->setVisible(state == State::Error);
        zoomLabel->setText(ready ? QString::number(qRound(canvas->zoom() * 100)) + "%" : QStringLiteral("—"));
        const auto label = index < 0 ? ImageViewer::tr("No image selected")
                                     : QString("%1 / %2  ·  %3")
                                           .arg(index + 1)
                                           .arg(items.size())
                                           .arg(items[index].title.isEmpty() ? id() : items[index].title);
        caption->setText(label.toHtmlEscaped());
        caption->setToolTip(label);
    }
    void changeState(State next, QString why = {}) {
        error = std::move(why);
        const bool changed = state != next;
        state = next;
        controls();
        if (changed)
            notify([owner = q, next] { emit owner->stateChanged(next); });
    }
    void renderCurrent() {
        if (index < 0) {
            canvas->setMessage(ImageViewer::tr("No images to display"));
            displayedId.clear();
            changeState(State::Empty);
            return;
        }
        const auto key = id();
        if (auto *image = cache.object(key)) {
            if (displayedId != key || canvas->image().cacheKey() != image->cacheKey())
                canvas->setImage(*image);
            displayedId = key;
            changeState(State::Ready);
        } else if (failures.contains(key)) {
            canvas->setMessage(failures.value(key));
            displayedId.clear();
            changeState(State::Error, failures.value(key));
        } else if (displayedId != key || canvas->image().isNull()) {
            canvas->setMessage(ImageViewer::tr("Loading image…"));
            displayedId.clear();
            changeState(State::Loading);
        }
    }
    void complete(const QString &key, detail::DecodeResult result) {
        pending.remove(key);
        if (result.error.isEmpty() && result.image.isNull())
            result.error = ImageViewer::tr("The image could not be decoded.");
        if (!result.error.isEmpty()) {
            failures.insert(key, result.error);
            if (id() == key)
                renderCurrent();
            notify([owner = q, key, reason = result.error] { emit owner->imageFailed(key, reason); });
            return;
        }
        result.image.setDevicePixelRatio(1.);
        cache.insert(key, new QImage(result.image), int((result.image.sizeInBytes() + 1023) / 1024));
        for (int row = 0; row < items.size(); ++row)
            if (items[row].id == key)
                thumbnails->item(row)->setIcon(thumbnailIcon(result.image));
        // The visible image is pinned even if it exceeds the optional cache budget.
        if (id() == key) {
            canvas->setImage(result.image);
            displayedId = key;
            changeState(State::Ready);
        }
        notify([owner = q, key, size = result.image.size()] { emit owner->imageLoaded(key, size); });
    }
    void pump() {
        while (active < 2 && !queue.isEmpty()) {
            const auto job = queue.dequeue();
            ++active;
            auto *watcher = new QFutureWatcher<detail::DecodeResult>(q);
            QObject::connect(watcher, &QFutureWatcher<detail::DecodeResult>::finished, q,
                             [this, watcher, job] {
                                 --active;
                                 const auto result = watcher->result();
                                 watcher->deleteLater();
                                 if (revision == job.revision)
                                     complete(job.item.id, result);
                                 pump();
                             });
            watcher->setFuture(QtConcurrent::run([job] {
                return job.cancelled->load() ? detail::DecodeResult{} : detail::decode(job.item, job.limits);
            }));
        }
    }
    void enqueue(ImageItem item, bool priority) {
        Job job{std::move(item), revision, limits, cancelled};
        if (priority)
            queue.prepend(job);
        else
            queue.enqueue(job);
        pump();
    }
    void ensure(int row, bool priority = false) {
        if (row < 0 || row >= items.size())
            return;
        const auto item = items[row];
        if (cache.contains(item.id) || pending.contains(item.id) || failures.contains(item.id))
            return;
        if (displayedId == item.id && !canvas->image().isNull())
            return;
        pending.insert(item.id);
        if (!item.filePath.isEmpty() || !item.encodedData.isEmpty() || !item.image.isNull())
            enqueue(item, priority);
        else {
            const auto token = ++sequence;
            external.insert(token, item.id);
            notify([this, token, key = item.id] {
                if (external.value(token) == key)
                    emit q->imageRequested(token, key);
            });
        }
    }
    void loadCurrent() {
        QSet<QString> needed;
        for (int row = std::max(0, index - limits.preloadRadius);
             row <= std::min(int(items.size()) - 1, index + limits.preloadRadius); ++row)
            needed.insert(items[row].id);
        for (auto it = external.begin(); it != external.end();) {
            if (needed.contains(it.value())) {
                ++it;
                continue;
            }
            const auto token = it.key();
            pending.remove(it.value());
            it = external.erase(it);
            QTimer::singleShot(0, q, [owner = q, token] { emit owner->requestCancelled(token); });
        }
        QQueue<Job> retained;
        while (!queue.isEmpty()) {
            auto job = queue.dequeue();
            if (needed.contains(job.item.id))
                retained.enqueue(job);
            else
                pending.remove(job.item.id);
        }
        queue = std::move(retained);
        renderCurrent();
        ensure(index, true);
        for (int distance = 1; distance <= limits.preloadRadius; ++distance) {
            ensure(index + distance);
            ensure(index - distance);
        }
    }
};

ImageViewer::ImageViewer(QWidget *parent) : QWidget(parent), d(std::make_unique<Private>(this)) {
    setObjectName("ketImageViewer");
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(8);
    d->caption = new QLabel(this);
    d->caption->setTextFormat(Qt::RichText);
    d->caption->setWordWrap(true);
    d->caption->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    layout->addWidget(d->caption);
    d->toolbar = new QToolBar(this);
    d->toolbar->setObjectName("imageToolbar");
    d->toolbar->setMovable(false);
    d->toolbar->setIconSize({20, 20});
    layout->addWidget(d->toolbar);
    d->canvas = new ImageCanvas(this);
    layout->addWidget(d->canvas, 1);
    d->thumbnails = new QListWidget(this);
    d->thumbnails->setObjectName("imageThumbnails");
    d->thumbnails->setAccessibleName(tr("Image thumbnails"));
    d->thumbnails->setViewMode(QListView::IconMode);
    d->thumbnails->setFlow(QListView::LeftToRight);
    d->thumbnails->setWrapping(false);
    d->thumbnails->setMovement(QListView::Static);
    d->thumbnails->setIconSize({72, 54});
    d->thumbnails->setGridSize({88, 78});
    d->thumbnails->setFixedHeight(96);
    layout->addWidget(d->thumbnails);
    auto action = [this](QString text, QString name, auto slot) {
        auto *result = d->toolbar->addAction(text);
        result->setObjectName(name);
        connect(result, &QAction::triggered, this, slot);
        return result;
    };
    d->back = action(tr("Previous"), "previousImage", [this] { previous(); });
    d->forward = action(tr("Next"), "nextImage", [this] { next(); });
    d->toolbar->addSeparator();
    d->zoomOut = action(tr("Zoom out"), "zoomOut", [this] { d->canvas->zoomOut(); });
    d->zoomLabel = new QLabel(this);
    d->zoomLabel->setMinimumWidth(52);
    d->zoomLabel->setAlignment(Qt::AlignCenter);
    d->toolbar->addWidget(d->zoomLabel);
    d->zoomIn = action(tr("Zoom in"), "zoomIn", [this] { d->canvas->zoomIn(); });
    d->fit = action(tr("Fit"), "fitImage", [this] { d->canvas->fit(); });
    d->actual = action(tr("100%"), "actualSize", [this] { d->canvas->actualSize(); });
    d->rotate = action(tr("Rotate"), "rotateImage", [this] { d->canvas->rotateClockwise(); });
    d->toolbar->addSeparator();
    d->copy = action(tr("Copy"), "copyImage", [this] { copyCurrent(); });
    d->save = action(tr("Save…"), "saveImage", [this] { emit saveRequested(currentId()); });
    d->retry = action(tr("Retry"), "retryImage", [this] { retry(); });
    action(tr("Close"), "closeViewer", [this] { emit closeRequested(); });
    connect(d->canvas, &ImageCanvas::viewChanged, this, [this] { d->controls(); });
    connect(d->canvas, &ImageCanvas::navigationRequested, this,
            [this](int delta) { setCurrentIndex(d->index + delta); });
    connect(d->thumbnails, &QListWidget::currentRowChanged, this, &ImageViewer::setCurrentIndex);
    d->renderCurrent();
}
ImageViewer::~ImageViewer() {
    d->cancelled->store(true);
    const auto tokens = d->external.keys();
    d->external.clear();
    for (auto token : tokens)
        emit requestCancelled(token);
}
bool ImageViewer::setItems(QList<ImageItem> items, int index) {
    QSet<QString> ids;
    for (const auto &item : items) {
        const int sources =
            int(!item.filePath.isEmpty()) + int(!item.encodedData.isEmpty()) + int(!item.image.isNull());
        if (item.id.isEmpty() || ids.contains(item.id) || sources > 1) {
            emit collectionRejected(
                tr("Each image needs a unique nonempty ID and at most one local source."));
            return false;
        }
        ids.insert(item.id);
    }
    if ((!items.isEmpty() && (index < 0 || index >= items.size())) || (items.isEmpty() && index != 0)) {
        emit collectionRejected(tr("The initial image index is out of range."));
        return false;
    }
    d->reset();
    d->items = std::move(items);
    d->index = -1;
    {
        const QSignalBlocker blocker(d->thumbnails);
        d->thumbnails->clear();
        for (const auto &item : d->items) {
            auto *entry = new QListWidgetItem(item.title.isEmpty() ? item.id : item.title, d->thumbnails);
            entry->setToolTip(entry->text());
            entry->setIcon(thumbnailIcon(item.thumbnail));
        }
    }
    if (d->items.isEmpty()) {
        d->renderCurrent();
        d->notify([this] { emit currentChanged(-1, {}); });
    } else
        setCurrentIndex(index);
    return true;
}
void ImageViewer::setCurrentIndex(int index) {
    if (index < 0 || index >= d->items.size() || index == d->index)
        return;
    d->index = index;
    {
        const QSignalBlocker blocker(d->thumbnails);
        d->thumbnails->setCurrentRow(index);
        d->thumbnails->scrollToItem(d->thumbnails->item(index));
    }
    d->loadCurrent();
    d->notify([this, index, id = d->id()] { emit currentChanged(index, id); });
}
int ImageViewer::count() const { return d->items.size(); }
int ImageViewer::currentIndex() const { return d->index; }
QString ImageViewer::currentId() const { return d->id(); }
ImageViewer::State ImageViewer::state() const { return d->state; }
QString ImageViewer::errorString() const { return d->error; }
ImageCanvas *ImageViewer::canvas() const { return d->canvas; }
LoadLimits ImageViewer::loadLimits() const { return d->limits; }
bool ImageViewer::setLoadLimits(LoadLimits limits) {
    if (limits.maximumEncodedBytes < 1 || limits.maximumEncodedBytes > 256LL * 1024 * 1024 ||
        limits.maximumPixels < 1 || limits.maximumPixels > 256LL * 1024 * 1024 || limits.cacheKiB < 1 ||
        limits.cacheKiB > 1024 * 1024 || limits.preloadRadius < 0 || limits.preloadRadius > 5)
        return false;
    d->limits = limits;
    d->reset();
    d->cache.setMaxCost(limits.cacheKiB);
    d->loadCurrent();
    return true;
}
void ImageViewer::setThumbnailsVisible(bool visible) { d->thumbnails->setVisible(visible); }
void ImageViewer::setToolbarVisible(bool visible) { d->toolbar->setVisible(visible); }
void ImageViewer::next() { setCurrentIndex(d->index + 1); }
void ImageViewer::previous() { setCurrentIndex(d->index - 1); }
void ImageViewer::clear() { setItems({}); }
void ImageViewer::retry() {
    if (d->index < 0 || d->state != State::Error)
        return;
    d->failures.remove(d->id());
    d->loadCurrent();
}
void ImageViewer::provideImage(quint64 requestId, QImage image) {
    if (!d->external.contains(requestId))
        return;
    const auto id = d->external.take(requestId);
    const auto error = detail::validateImage(image, d->limits);
    d->complete(id, {error.isEmpty() ? image : QImage(), error});
}
void ImageViewer::provideData(quint64 requestId, QByteArray data) {
    if (!d->external.contains(requestId))
        return;
    const auto id = d->external.take(requestId);
    ImageItem item;
    item.id = id;
    item.encodedData = std::move(data);
    d->enqueue(std::move(item), id == d->id());
}
void ImageViewer::failRequest(quint64 requestId, QString reason) {
    if (!d->external.contains(requestId))
        return;
    const auto id = d->external.take(requestId);
    d->complete(id, {{}, reason.isEmpty() ? tr("The image could not be loaded.") : reason});
}
void ImageViewer::copyCurrent() {
    if (d->state == State::Ready)
        QApplication::clipboard()->setImage(d->canvas->displayedImage());
}
bool ImageViewer::saveCurrent(const QString &path, QString *error) const {
    if (error)
        error->clear();
    const auto fail = [error](QString message) {
        if (error)
            *error = std::move(message);
        return false;
    };
    if (d->state != State::Ready)
        return fail(tr("No image is ready to save."));
    const auto format = QFileInfo(path).suffix().toLatin1().toLower();
    if (!QImageWriter::supportedImageFormats().contains(format))
        return fail(tr("Choose a supported image file extension."));
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly))
        return fail(file.errorString());
    QImageWriter writer(&file, format);
    if (!writer.write(d->canvas->displayedImage()))
        return fail(writer.errorString());
    if (!file.commit())
        return fail(file.errorString());
    return true;
}
} // namespace ket::imageviewer
