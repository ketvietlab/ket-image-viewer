// SPDX-License-Identifier: MIT
#include <algorithm>
#include <cmath>
#include <ket/imageviewer/viewport.h>
namespace ket::imageviewer {
QSizeF Viewport::orientedSize() const { return turns_ % 2 ? image_.transposed() : image_; }
double Viewport::fitScale() const {
    const auto size = orientedSize();
    if (size.isEmpty() || view_.isEmpty())
        return 1.;
    return std::min({1., view_.width() / size.width(), view_.height() / size.height()});
}
void Viewport::setImageSize(QSizeF size) {
    image_ = size;
    turns_ = 0;
    fit();
}
void Viewport::setViewSize(QSizeF size) {
    view_ = size;
    applyMode();
}
void Viewport::applyMode() {
    if (mode_ == Mode::Fit) {
        scale_ = fitScale();
        offset_ = {};
    } else if (mode_ == Mode::ActualSize)
        scale_ = 1.;
    constrain();
}
void Viewport::fit() {
    mode_ = Mode::Fit;
    offset_ = {};
    applyMode();
}
void Viewport::actualSize() {
    mode_ = Mode::ActualSize;
    offset_ = {};
    applyMode();
}
QTransform Viewport::transform() const {
    QTransform result;
    result.translate(view_.width() / 2 + offset_.x(), view_.height() / 2 + offset_.y());
    result.scale(scale_, scale_);
    result.rotate(rotation());
    result.translate(-image_.width() / 2, -image_.height() / 2);
    return result;
}
QRectF Viewport::bounds() const { return transform().mapRect(QRectF(QPointF(), image_)); }
QPointF Viewport::imagePoint(QPointF point) const { return transform().inverted().map(point); }
void Viewport::zoomAt(double factor, QPointF anchor) {
    if (image_.isEmpty() || !std::isfinite(factor) || factor <= 0.)
        return;
    const auto fixedPixel = imagePoint(anchor);
    // Include a fit scale smaller than 1% for very large inputs; never trap the user outside Fit.
    scale_ = std::clamp(scale_ * factor, std::min(.01, fitScale()), 32.);
    mode_ = Mode::Custom;
    offset_ += anchor - transform().map(fixedPixel);
    constrain();
}
void Viewport::constrain() {
    const auto size = orientedSize() * scale_;
    const double x = std::max(0., (size.width() - view_.width()) / 2);
    const double y = std::max(0., (size.height() - view_.height()) / 2);
    offset_.setX(std::clamp(offset_.x(), -x, x));
    offset_.setY(std::clamp(offset_.y(), -y, y));
}
void Viewport::panBy(QPointF delta) {
    if (!std::isfinite(delta.x()) || !std::isfinite(delta.y()))
        return;
    offset_ += delta;
    constrain();
}
void Viewport::rotateClockwise() {
    turns_ = (turns_ + 1) % 4;
    offset_ = QPointF(-offset_.y(), offset_.x());
    applyMode();
}
} // namespace ket::imageviewer
