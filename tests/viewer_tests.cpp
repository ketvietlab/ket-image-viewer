// SPDX-License-Identifier: MIT
#include <QAction>
#include <QApplication>
#include <QBuffer>
#include <QClipboard>
#include <QFile>
#include <QImageReader>
#include <QNativeGestureEvent>
#include <QPointer>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QWheelEvent>
#include <QtTest>
#include <cmath>
#include <ket/imageviewer/image_viewer.h>
using namespace ket::imageviewer;
namespace {
QImage solid(QSize size = {800, 600}, QColor color = Qt::red) {
    QImage image(size, QImage::Format_ARGB32);
    image.fill(color);
    return image;
}
ImageItem ready(QString id, QSize size = {800, 600}, QColor color = Qt::red) {
    return {id, id, {}, {}, solid(size, color), {}};
}
QByteArray png(const QImage &image) {
    QByteArray data;
    QBuffer buffer(&data);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "PNG");
    return data;
}
} // namespace
class ViewerTests : public QObject {
    Q_OBJECT
  private slots:
    void fitZoomAnchorPanAndRotation() {
        Viewport view;
        view.setViewSize({1000, 750});
        view.setImageSize({4000, 3000});
        QCOMPARE(view.scale(), .25);
        QCOMPARE(view.bounds(), QRectF(0, 0, 1000, 750));
        const QPointF anchor(400, 300);
        const auto before = view.imagePoint(anchor);
        view.zoomAt(2., anchor);
        QCOMPARE(view.scale(), .5);
        QVERIFY(QLineF(before, view.imagePoint(anchor)).length() < 1e-6);
        view.panBy({1e6, 1e6});
        QCOMPARE(view.bounds().topLeft(), QPointF(0, 0));
        view.panBy({-2e6, -2e6});
        QCOMPARE(view.bounds().bottomRight(), QPointF(1000, 750));
        view.fit();
        view.rotateClockwise();
        QCOMPARE(view.rotation(), 90);
        QCOMPARE(view.bounds().height(), 750.);
        QCOMPARE(view.bounds().width(), 562.5);
        view.setViewSize({600, 800});
        QCOMPARE(view.bounds(), QRectF(0, 0, 600, 800));
        for (int i = 0; i < 3; ++i)
            view.rotateClockwise();
        QCOMPARE(view.rotation(), 0);
    }
    void invalidZoomAndSmallImagesStayFinite() {
        Viewport view;
        view.setViewSize({800, 600});
        view.setImageSize({100, 50});
        QCOMPARE(view.scale(), 1.);
        view.zoomAt(0., {});
        view.zoomAt(NAN, {});
        QCOMPARE(view.scale(), 1.);
        view.zoomAt(1e100, {});
        QCOMPARE(view.scale(), 32.);
        view.zoomAt(1e-100, {});
        QCOMPARE(view.scale(), .01);
        view.actualSize();
        QCOMPARE(view.mode(), Viewport::Mode::ActualSize);
        view.setImageSize({});
        QVERIFY(std::isfinite(view.scale()));
    }
    void canvasWheelPinchKeyboardAndRotatedPixels() {
        ImageCanvas canvas;
        canvas.resize(800, 600);
        canvas.show();
        canvas.setImage(solid({1600, 1200}));
        const QPointF anchor(300, 250);
        const auto before = canvas.viewport().imagePoint(anchor);
        QWheelEvent wheel(anchor, canvas.mapToGlobal(anchor.toPoint()), {}, {0, 120}, Qt::NoButton,
                          Qt::NoModifier, Qt::NoScrollPhase, false);
        QApplication::sendEvent(&canvas, &wheel);
        QCOMPARE(canvas.zoom(), .625);
        QVERIFY(QLineF(before, canvas.viewport().imagePoint(anchor)).length() < 1e-6);
        QNativeGestureEvent pinch(Qt::ZoomNativeGesture, QPointingDevice::primaryPointingDevice(), 2, anchor,
                                  anchor, anchor, std::log(2.), {});
        QApplication::sendEvent(&canvas, &pinch);
        QVERIFY(qAbs(canvas.zoom() - 1.25) < 1e-6);
        QTest::keyClick(&canvas, Qt::Key_R);
        QCOMPARE(canvas.displayedImage().size(), QSize(1200, 1600));
        QTest::keyClick(&canvas, Qt::Key_F);
        QCOMPARE(canvas.viewport().mode(), Viewport::Mode::Fit);
        QTest::mouseDClick(&canvas, Qt::LeftButton);
        QCOMPARE(canvas.viewport().mode(), Viewport::Mode::ActualSize);
    }
    void collectionValidationIsAtomicAndNavigationDoesNotWrap() {
        ImageViewer viewer;
        QVERIFY(viewer.setItems({ready("a"), ready("b")}));
        QTRY_COMPARE(viewer.state(), ImageViewer::State::Ready);
        QVERIFY(!viewer.setItems({ready("duplicate"), ready("duplicate")}));
        QCOMPARE(viewer.currentId(), QString("a"));
        QVERIFY(!viewer.setItems({ready("")}));
        QVERIFY(!viewer.setItems({ready("other")}, 5));
        viewer.previous();
        QCOMPARE(viewer.currentIndex(), 0);
        viewer.next();
        QCOMPARE(viewer.currentIndex(), 1);
        viewer.next();
        QCOMPARE(viewer.currentIndex(), 1);
        QTRY_COMPARE(viewer.state(), ImageViewer::State::Ready);
        viewer.clear();
        QCOMPARE(viewer.state(), ImageViewer::State::Empty);
        QVERIFY(viewer.canvas()->image().isNull());
        QVERIFY(!viewer.findChild<QAction *>("copyImage")->isEnabled());
    }
    void bytesFilesAndLimitsHaveRealErrorStates() {
        QTemporaryDir dir;
        const auto file = dir.filePath("test.png");
        QVERIFY(solid().save(file));
        ImageViewer viewer;
        QVERIFY(viewer.setItems({{"file", "File", file, {}, {}, {}}}));
        QTRY_COMPARE(viewer.state(), ImageViewer::State::Ready);
        QVERIFY(viewer.setItems({{"bytes", "Bytes", {}, png(solid({333, 222})), {}, {}}}));
        QTRY_COMPARE(viewer.canvas()->image().size(), QSize(333, 222));
        auto limits = viewer.loadLimits();
        limits.maximumPixels = 10;
        QVERIFY(viewer.setLoadLimits(limits));
        QTRY_COMPARE(viewer.state(), ImageViewer::State::Error);
        QVERIFY(!viewer.errorString().isEmpty());
        limits.maximumPixels = 1000000;
        limits.maximumEncodedBytes = 2;
        QVERIFY(viewer.setLoadLimits(limits));
        QTRY_COMPARE(viewer.state(), ImageViewer::State::Error);
        limits.maximumEncodedBytes = 1000000;
        QVERIFY(viewer.setLoadLimits(limits));
        QVERIFY(viewer.setItems({{"bad", "Bad", {}, QByteArray("invalid image"), {}, {}}}));
        QTRY_COMPARE(viewer.state(), ImageViewer::State::Error);
        QVERIFY(viewer.canvas()->image().isNull());
        QVERIFY(viewer.findChild<QAction *>("retryImage")->isVisible());
    }
    void providerTokensRejectStaleResultsAndCanRetry() {
        ImageViewer viewer;
        auto limits = viewer.loadLimits();
        limits.preloadRadius = 0;
        viewer.setLoadLimits(limits);
        QSignalSpy requests(&viewer, &ImageViewer::imageRequested),
            cancelled(&viewer, &ImageViewer::requestCancelled);
        viewer.setItems({{"old", "Old"}});
        QTRY_COMPARE(requests.size(), 1);
        const auto old = requests[0][0].toULongLong();
        viewer.setItems({{"new", "New"}});
        QTRY_COMPARE(requests.size(), 2);
        QTRY_COMPARE(cancelled.size(), 1);
        viewer.provideImage(old, solid());
        QCOMPARE(viewer.state(), ImageViewer::State::Loading);
        const auto token = requests[1][0].toULongLong();
        viewer.failRequest(token, "Network offline");
        QCOMPARE(viewer.state(), ImageViewer::State::Error);
        QCOMPARE(viewer.errorString(), QString("Network offline"));
        viewer.retry();
        QTRY_COMPARE(requests.size(), 3);
        viewer.provideData(requests[2][0].toULongLong(), png(solid({321, 123}, Qt::green)));
        QTRY_COMPARE(viewer.state(), ImageViewer::State::Ready);
        QCOMPARE(viewer.canvas()->image().size(), QSize(321, 123));
        viewer.provideImage(token, solid());
        QCOMPARE(viewer.canvas()->image().size(), QSize(321, 123));
    }
    void preloadCancelsRequestsOutsideTheActiveNeighbourhood() {
        ImageViewer viewer;
        QSignalSpy requests(&viewer, &ImageViewer::imageRequested),
            cancelled(&viewer, &ImageViewer::requestCancelled);
        QList<ImageItem> items;
        for (int i = 0; i < 10; ++i)
            items.append({QString::number(i), QString::number(i)});
        viewer.setItems(items);
        QTRY_COMPARE(requests.size(), 2);
        viewer.setCurrentIndex(8);
        QTRY_COMPARE(requests.size(), 5);
        QTRY_COMPARE(cancelled.size(), 2);
        viewer.provideImage(requests[0][0].toULongLong(), solid());
        QCOMPARE(viewer.state(), ImageViewer::State::Loading);
        for (const auto &request : requests)
            if (request[1].toString() == "8")
                viewer.provideImage(request[0].toULongLong(), solid({444, 222}));
        QCOMPARE(viewer.state(), ImageViewer::State::Ready);
        QCOMPARE(viewer.canvas()->image().size(), QSize(444, 222));
    }
    void rotationCopyAndAtomicExportKeepDimensionsAndAlpha() {
        QTemporaryDir dir;
        ImageViewer viewer;
        auto input = solid({180, 90}, QColor(100, 150, 200, 100));
        viewer.setItems({{"alpha", "Alpha", {}, {}, input, {}}});
        QTRY_COMPARE(viewer.state(), ImageViewer::State::Ready);
        viewer.canvas()->rotateClockwise();
        viewer.copyCurrent();
        QCOMPARE(QApplication::clipboard()->image().size(), QSize(90, 180));
        QString error;
        QVERIFY2(viewer.saveCurrent(dir.filePath("rotated.png"), &error), qPrintable(error));
        QImage exported(dir.filePath("rotated.png"));
        QCOMPARE(exported.size(), QSize(90, 180));
        QCOMPARE(exported.pixelColor(0, 0).alpha(), 100);
        QVERIFY(!viewer.saveCurrent(dir.filePath("unsupported.xyz"), &error));
        QVERIFY(!error.isEmpty());
        viewer.clear();
        QVERIFY(!viewer.saveCurrent(dir.filePath("empty.png"), &error));
    }
    void evictedVisibleImageRemainsUsableAndReloadsOnReturn() {
        ImageViewer viewer;
        auto limits = viewer.loadLimits();
        limits.cacheKiB = 1;
        limits.preloadRadius = 0;
        viewer.setLoadLimits(limits);
        viewer.setItems({ready("one", {700, 400}), ready("two", {300, 600})});
        QTRY_COMPARE(viewer.state(), ImageViewer::State::Ready);
        QCOMPARE(viewer.canvas()->image().size(), QSize(700, 400));
        viewer.next();
        QTRY_COMPARE(viewer.canvas()->image().size(), QSize(300, 600));
        viewer.previous();
        QTRY_COMPARE(viewer.canvas()->image().size(), QSize(700, 400));
    }
    void clearingOrDeletingDuringDecodeDoesNotRestoreOldData() {
        ImageViewer viewer;
        const auto data = png(solid({2000, 2000}));
        viewer.setItems({{"large", "Large", {}, data, {}, {}}});
        viewer.clear();
        QTest::qWait(100);
        QCOMPARE(viewer.state(), ImageViewer::State::Empty);
        QVERIFY(viewer.canvas()->image().isNull());
        auto *pending = new ImageViewer;
        pending->setItems({{"large", "Large", {}, data, {}, {}}});
        delete pending;
        QTest::qWait(50);
    }
    void completionSlotMayClearTheCollection() {
        ImageViewer viewer;
        QObject::connect(&viewer, &ImageViewer::imageLoaded, &viewer, [&] { viewer.clear(); });
        viewer.setItems({ready("clear-on-completion")});
        QTRY_COMPARE(viewer.state(), ImageViewer::State::Empty);
        QCOMPARE(viewer.count(), 0);
    }
};
QTEST_MAIN(ViewerTests)
#include "viewer_tests.moc"
