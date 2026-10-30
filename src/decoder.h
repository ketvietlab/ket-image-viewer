// SPDX-License-Identifier: MIT
#pragma once
#include <ket/imageviewer/image_viewer.h>
namespace ket::imageviewer::detail {
struct DecodeResult {
    QImage image;
    QString error;
};
DecodeResult decode(const ImageItem &item, const LoadLimits &limits);
QString validateImage(const QImage &image, const LoadLimits &limits);
} // namespace ket::imageviewer::detail
