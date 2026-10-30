// SPDX-License-Identifier: MIT
#pragma once
#include <QByteArray>
#include <QList>
#include <ket/imageviewer/image_canvas.h>
#include <memory>
namespace ket::imageviewer {
// Exactly one local source may be supplied. No local source means host-provided data.
struct ImageItem {
    QString id;
    QString title;
    QString filePath;
    QByteArray encodedData;
    QImage image;
    QImage thumbnail;
};
struct LoadLimits {
    qint64 maximumEncodedBytes = 64 * 1024 * 1024;
    qint64 maximumPixels = 24 * 1024 * 1024;
    int cacheKiB = 128 * 1024;
    int preloadRadius = 1;
};
class KET_IMAGE_VIEWER_EXPORT ImageViewer : public QWidget {
    Q_OBJECT
  public:
    enum class State { Empty, Loading, Ready, Error };
    Q_ENUM(State)
    explicit ImageViewer(QWidget *parent = nullptr);
    ~ImageViewer() override;
    // Invalid IDs/source combinations reject atomically; the existing collection stays visible.
    bool setItems(QList<ImageItem> items, int index = 0);
    int count() const;
    int currentIndex() const;
    QString currentId() const;
    State state() const;
    QString errorString() const;
    ImageCanvas *canvas() const;
    LoadLimits loadLimits() const;
    bool setLoadLimits(LoadLimits limits); // Resets pending loads/cache if changed.
    void setThumbnailsVisible(bool visible);
    void setToolbarVisible(bool visible);
    // Thread affinity: all public calls, including provider completion, are GUI-thread calls.
    // Completion IDs are one-use tokens. Unknown/cancelled IDs are ignored.
    void provideImage(quint64 requestId, QImage image);
    void provideData(quint64 requestId, QByteArray data);
    void failRequest(quint64 requestId, QString reason);
    bool saveCurrent(const QString &path, QString *error = nullptr) const; // Decoded image + view rotation.
  public slots:
    void setCurrentIndex(int index);
    void next();
    void previous();
    void retry();
    void clear(); // Invalidate all provider tokens, queued decodes and cached image data.
    void copyCurrent();
  signals:
    void currentChanged(int index, const QString &id);
    void stateChanged(ket::imageviewer::ImageViewer::State state);
    void collectionRejected(const QString &reason);
    void imageRequested(quint64 requestId, const QString &id);
    void requestCancelled(quint64 requestId);
    void imageLoaded(const QString &id, QSize size);
    void imageFailed(const QString &id, const QString &reason);
    void saveRequested(const QString &id);
    void closeRequested();

  private:
    class Private;
    std::unique_ptr<Private> d;
};
} // namespace ket::imageviewer
Q_DECLARE_METATYPE(ket::imageviewer::ImageViewer::State)
