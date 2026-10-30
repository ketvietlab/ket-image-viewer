// SPDX-License-Identifier: MIT
#include <QKeyEvent>
#include <QMouseEvent>
#include <QNativeGestureEvent>
#include <QPainter>
#include <QWheelEvent>
#include <cmath>
#include <ket/imageviewer/image_canvas.h>
namespace ket::imageviewer {
ImageCanvas::ImageCanvas(QWidget *parent) : QWidget(parent) {
    setObjectName("ketImageCanvas");
    setFocusPolicy(Qt::StrongFocus);
    setMinimumSize(160, 120);
    setAccessibleName(tr("Image canvas"));
    setAccessibleDescription(
        tr("Scroll to zoom, drag to pan. Double-click switches fit and 100%. Arrow keys switch images."));
}
void ImageCanvas::setImage(QImage image) {
    image_ = std::move(image);
    // Geometry is in source pixels, independent of a caller-provided QImage DPR tag.
    image_.setDevicePixelRatio(1.);
    message_.clear();
    viewport_.setImageSize(image_.size());
    refresh();
}
QImage ImageCanvas::image() const { return image_; }
QImage ImageCanvas::displayedImage() const {
    return viewport_.rotation() ? image_.transformed(QTransform().rotate(viewport_.rotation())) : image_;
}
void ImageCanvas::setMessage(QString message) {
    setImage({});
    message_ = std::move(message);
    update();
}
void ImageCanvas::refresh() {
    update();
    emit viewChanged();
}
void ImageCanvas::fit() {
    viewport_.fit();
    refresh();
}
void ImageCanvas::actualSize() {
    viewport_.actualSize();
    refresh();
}
void ImageCanvas::zoomIn() {
    viewport_.zoomAt(1.25, rect().center());
    refresh();
}
void ImageCanvas::zoomOut() {
    viewport_.zoomAt(.8, rect().center());
    refresh();
}
void ImageCanvas::rotateClockwise() {
    viewport_.rotateClockwise();
    refresh();
}
void ImageCanvas::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    const auto background = palette().color(QPalette::Base);
    painter.fillRect(rect(), background);
    if (image_.isNull()) {
        painter.setPen(palette().color(QPalette::Text));
        painter.drawText(rect().adjusted(24, 24, -24, -24), Qt::AlignCenter | Qt::TextWordWrap, message_);
    } else {
        painter.save();
        painter.setClipRect(viewport_.bounds());
        if (image_.hasAlphaChannel()) {
            const auto alternate = palette().color(QPalette::AlternateBase);
            for (int y = 0; y < height(); y += 12)
                for (int x = 0; x < width(); x += 12)
                    if ((x / 12 + y / 12) % 2)
                        painter.fillRect(QRect(x, y, 12, 12), alternate);
        }
        painter.setRenderHint(QPainter::SmoothPixmapTransform);
        painter.setTransform(viewport_.transform());
        painter.drawImage(QPointF(), image_);
        painter.restore();
    }
    if (hasFocus()) {
        painter.setPen(QPen(palette().color(QPalette::Highlight), 1));
        painter.drawRect(rect().adjusted(0, 0, -1, -1));
    }
}
void ImageCanvas::resizeEvent(QResizeEvent *) {
    viewport_.setViewSize(size());
    refresh();
}
void ImageCanvas::mousePressEvent(QMouseEvent *e) {
    if (e->button() == Qt::LeftButton && !image_.isNull()) {
        setFocus();
        dragging_ = true;
        lastPointer_ = e->position();
        setCursor(Qt::ClosedHandCursor);
        e->accept();
    } else
        QWidget::mousePressEvent(e);
}
void ImageCanvas::mouseMoveEvent(QMouseEvent *e) {
    if (!dragging_)
        return;
    viewport_.panBy(e->position() - lastPointer_);
    lastPointer_ = e->position();
    refresh();
    e->accept();
}
void ImageCanvas::mouseReleaseEvent(QMouseEvent *e) {
    if (e->button() == Qt::LeftButton) {
        dragging_ = false;
        unsetCursor();
        e->accept();
    }
}
void ImageCanvas::mouseDoubleClickEvent(QMouseEvent *e) {
    if (e->button() != Qt::LeftButton || image_.isNull())
        return;
    viewport_.mode() == Viewport::Mode::Fit ? actualSize() : fit();
    e->accept();
}
void ImageCanvas::wheelEvent(QWheelEvent *e) {
    if (image_.isNull()) {
        e->ignore();
        return;
    }
    if (!e->pixelDelta().isNull() && !(e->modifiers() & Qt::ControlModifier))
        viewport_.panBy(e->pixelDelta());
    else {
        const double steps =
            e->angleDelta().isNull() ? e->pixelDelta().y() / 40. : e->angleDelta().y() / 120.;
        viewport_.zoomAt(std::pow(1.25, steps), e->position());
    }
    refresh();
    e->accept();
}
void ImageCanvas::keyPressEvent(QKeyEvent *e) {
    if (e->modifiers() != Qt::NoModifier) {
        QWidget::keyPressEvent(e);
        return;
    }
    switch (e->key()) {
    case Qt::Key_Left:
        emit navigationRequested(-1);
        break;
    case Qt::Key_Right:
        emit navigationRequested(1);
        break;
    case Qt::Key_Plus:
    case Qt::Key_Equal:
        zoomIn();
        break;
    case Qt::Key_Minus:
        zoomOut();
        break;
    case Qt::Key_0:
        actualSize();
        break;
    case Qt::Key_F:
        fit();
        break;
    case Qt::Key_R:
        rotateClockwise();
        break;
    default:
        QWidget::keyPressEvent(e);
        return;
    }
    e->accept();
}
bool ImageCanvas::event(QEvent *e) {
    if (e->type() == QEvent::NativeGesture && !image_.isNull()) {
        auto *gesture = static_cast<QNativeGestureEvent *>(e);
        if (gesture->gestureType() == Qt::ZoomNativeGesture) {
            viewport_.zoomAt(std::exp(gesture->value()), gesture->position());
            refresh();
            e->accept();
            return true;
        }
    }
    if (e->type() == QEvent::UngrabMouse || e->type() == QEvent::WindowDeactivate) {
        dragging_ = false;
        unsetCursor();
    }
    return QWidget::event(e);
}
} // namespace ket::imageviewer
