// SPDX-License-Identifier: MIT
#pragma once
#include <QPointF>
#include <QSizeF>
#include <QTransform>
#include <ket/imageviewer/export.h>
namespace ket::imageviewer {
// Image coordinates are source pixels. View coordinates are Qt logical pixels.
class KET_IMAGE_VIEWER_EXPORT Viewport {
  public:
    enum class Mode { Fit, ActualSize, Custom };
    void setImageSize(QSizeF size);
    void setViewSize(QSizeF size);
    void fit();
    void actualSize();
    void zoomAt(double factor, QPointF anchor);
    void panBy(QPointF delta);
    void rotateClockwise();
    QTransform transform() const;
    QRectF bounds() const;
    QPointF imagePoint(QPointF viewPoint) const;
    double scale() const { return scale_; }
    int rotation() const { return turns_ * 90; }
    Mode mode() const { return mode_; }
    QSizeF imageSize() const { return image_; }

  private:
    QSizeF orientedSize() const;
    double fitScale() const;
    void constrain();
    void applyMode();
    QSizeF image_, view_;
    QPointF offset_;
    double scale_ = 1.;
    int turns_ = 0;
    Mode mode_ = Mode::Fit;
};
} // namespace ket::imageviewer
