// SPDX-License-Identifier: MIT
#include "decoder.h"
#include <QBuffer>
#include <QFile>
#include <QImageReader>
namespace ket::imageviewer::detail {
QString validateImage(const QImage &image, const LoadLimits &limits) {
    if (image.isNull())
        return ImageViewer::tr("The image could not be decoded.");
    if (qint64(image.width()) * image.height() > limits.maximumPixels)
        return ImageViewer::tr("The image exceeds the pixel limit.");
    return {};
}
DecodeResult decode(const ImageItem &item, const LoadLimits &limits) {
    if (!item.image.isNull())
        return {item.image, validateImage(item.image, limits)};
    auto bytes = item.encodedData;
    if (!item.filePath.isEmpty()) {
        QFile file(item.filePath);
        if (!file.open(QIODevice::ReadOnly))
            return {{}, file.errorString()};
        if (file.size() > limits.maximumEncodedBytes)
            return {{}, ImageViewer::tr("The file exceeds the size limit.")};
        bytes = file.read(limits.maximumEncodedBytes + 1);
        if (file.error() != QFileDevice::NoError)
            return {{}, file.errorString()};
    }
    if (bytes.size() > limits.maximumEncodedBytes)
        return {{}, ImageViewer::tr("The file exceeds the size limit.")};
    QBuffer buffer(&bytes);
    buffer.open(QIODevice::ReadOnly);
    QImageReader reader(&buffer);
    reader.setAutoTransform(true);
    const auto size = reader.size();
    if (!size.isValid())
        return {{}, reader.errorString()};
    if (qint64(size.width()) * size.height() > limits.maximumPixels)
        return {{}, ImageViewer::tr("The image exceeds the pixel limit.")};
    auto image = reader.read(); // v0.1 intentionally displays the first frame of animated formats.
    if (image.isNull())
        return {{}, reader.errorString()};
    const auto error = validateImage(image, limits);
    return {error.isEmpty() ? image : QImage(), error};
}
} // namespace ket::imageviewer::detail
