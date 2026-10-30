// SPDX-License-Identifier: MIT
#pragma once
#include <QImage>
#include <QWidget>
#include <ket/imageviewer/viewport.h>
namespace ket::imageviewer {
class KET_IMAGE_VIEWER_EXPORT ImageCanvas : public QWidget {
    Q_OBJECT
  public:
    explicit ImageCanvas(QWidget *parent = nullptr);
    void setImage(QImage image);
    QImage image() const;
    QImage displayedImage() const;
    void setMessage(QString message);
    const Viewport &viewport() const { return viewport_; }
    double zoom() const { return viewport_.scale(); }
  public slots:
    void fit();
    void actualSize();
    void zoomIn();
    void zoomOut();
    void rotateClockwise();
  signals:
    void viewChanged();
    void navigationRequested(int delta);

  protected:
    void paintEvent(QPaintEvent *) override;
    void resizeEvent(QResizeEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;
    void mouseDoubleClickEvent(QMouseEvent *) override;
    void wheelEvent(QWheelEvent *) override;
    void keyPressEvent(QKeyEvent *) override;
    bool event(QEvent *) override;

  private:
    void refresh();
    Viewport viewport_;
    QImage image_;
    QString message_;
    QPointF lastPointer_;
    bool dragging_ = false;
};
} // namespace ket::imageviewer
