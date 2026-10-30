// SPDX-License-Identifier: MIT
#include <QApplication>
#include <QCommandLineParser>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontDatabase>
#include <QMainWindow>
#include <QMessageBox>
#include <QPainter>
#include <QShortcut>
#include <QStatusBar>
#include <QTimer>
#include <QToolBar>
#include <ket/imageviewer/image_viewer.h>
using namespace ket::imageviewer;
static QList<ImageItem> samples() {
    QList<ImageItem> result;
    QImage landscape(2400, 1600, QImage::Format_RGB32);
    {
        QPainter p(&landscape);
        p.fillRect(landscape.rect(), QColor("#f0ede6"));
        p.setPen(QColor("#272a36"));
        p.setFont(QFont(QApplication::font().family(), 92, QFont::DemiBold));
        p.drawText(QRect(140, 100, 2100, 160), Qt::AlignVCenter, "A closer look.");
        p.setFont(QFont(QApplication::font().family(), 32));
        p.drawText(145, 335, "KÉT IMAGE VIEWER   /   2400 × 1600 PX");
        const QList<QColor> colors{QColor("#5658b8"), QColor("#d4ad65"), QColor("#90a79a"),
                                   QColor("#d08476")};
        for (int i = 0; i < 4; ++i) {
            const QRect block(140 + i * 540, 450, 500, 850);
            p.fillRect(block, colors[i]);
            p.setPen(QPen(QColor(255, 255, 255, 155), 3));
            for (int y = block.top() + 50; y < block.bottom(); y += 50)
                p.drawLine(block.left() + 30, y, block.right() - 30, y);
            p.setBrush(colors[i].lighter(125));
            p.setPen(Qt::NoPen);
            p.drawEllipse(block.center(), 180, 180);
            p.setPen(Qt::white);
            p.setFont(QFont(QApplication::font().family(), 96, QFont::Bold));
            p.drawText(block, Qt::AlignCenter, QString::number(i + 1));
        }
        p.setPen(QColor("#555864"));
        p.setFont(QFont(QApplication::font().family(), 28));
        p.drawText(145, 1440, "SCROLL TO ZOOM  ·  DRAG TO EXPLORE  ·  R TO ROTATE");
    }
    result.append({"sample-landscape", "Color study · 2400 × 1600", {}, {}, landscape, {}});
    QImage portrait(1000, 2400, QImage::Format_RGB32);
    {
        QPainter p(&portrait);
        p.fillRect(portrait.rect(), QColor("#fbf9f5"));
        p.setPen(QColor("#292d3e"));
        p.setFont(QFont(QApplication::font().family(), 52, QFont::DemiBold));
        p.drawText(80, 150, "Room for detail.");
        p.setFont(QFont(QApplication::font().family(), 24));
        p.drawText(80, 215, "PORTRAIT TEST  /  1000 × 2400 PX");
        for (int i = 0; i < 12; ++i) {
            const int y = 310 + i * 150;
            p.setPen(QColor("#5658b8"));
            p.drawText(80, y, QString("%1").arg(i + 1, 2, 10, QChar('0')));
            p.setPen(QColor("#3c4050"));
            p.drawText(160, y, "Zoom, pan, rotate and fit");
            p.setPen(QColor("#dedde3"));
            p.drawLine(80, y + 45, 920, y + 45);
        }
        p.setPen(QColor("#5658b8"));
        p.drawText(80, 2250, "End of portrait — nothing cropped.");
    }
    result.append({"sample-portrait", "Portrait · 1000 × 2400", {}, {}, portrait, {}});
    QImage alpha(1400, 1000, QImage::Format_ARGB32_Premultiplied);
    alpha.fill(Qt::transparent);
    {
        QPainter p(&alpha);
        p.setRenderHint(QPainter::Antialiasing);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(86, 88, 184, 210));
        p.drawEllipse(QRectF(180, 140, 650, 650));
        p.setBrush(QColor(144, 167, 154, 190));
        p.drawEllipse(QRectF(570, 200, 650, 650));
        p.setPen(Qt::white);
        p.setFont(QFont(QApplication::font().family(), 60, QFont::DemiBold));
        p.drawText(alpha.rect(), Qt::AlignCenter, "TRANSPARENCY");
    }
    result.append({"sample-alpha", "Transparency · RGBA", {}, {}, alpha, {}});
    result.append({"sample-error",
                   "Missing image · retry example",
                   "/ket-image-viewer-example/does-not-exist.png",
                   {},
                   {},
                   {}});
    return result;
}
static void theme(QApplication &app, bool dark) {
    QPalette palette;
    palette.setColor(QPalette::Window, QColor(dark ? "#1c1d24" : "#f7f7fa"));
    palette.setColor(QPalette::WindowText, QColor(dark ? "#eeeef4" : "#282a37"));
    palette.setColor(QPalette::Base, QColor(dark ? "#121319" : "#ffffff"));
    palette.setColor(QPalette::AlternateBase, QColor(dark ? "#292b35" : "#edeef3"));
    palette.setColor(QPalette::Text, palette.windowText().color());
    palette.setColor(QPalette::Button, palette.window().color());
    palette.setColor(QPalette::ButtonText, palette.windowText().color());
    palette.setColor(QPalette::Highlight, QColor("#5658b8"));
    palette.setColor(QPalette::HighlightedText, Qt::white);
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(dark ? "#737582" : "#8d8f9b"));
    app.setPalette(palette);
}
int main(int argc, char **argv) {
    QApplication app(argc, argv);
    // The demo owns its style; the library always inherits the host application style.
    app.setStyle("Fusion");
    app.setFont(QFontDatabase::systemFont(QFontDatabase::GeneralFont));
    app.setApplicationName("Két Image Viewer");
    app.setOrganizationName("Két");
    app.setApplicationVersion("0.1.0");
    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addOption({"dark", "Use the dark demo palette"});
    parser.addOption(
        {"font", "Optional locally installed Két Sans TTF (not distributed by this repo)", "PATH"});
    parser.addOption({"capture", "Save a widget screenshot and exit (QA)", "PATH"});
    parser.addOption({"width", "Initial demo width", "PIXELS", "1120"});
    parser.addPositionalArgument("images", "Local images to view", "[images...]");
    parser.process(app);
    if (parser.isSet("font")) {
        const int id = QFontDatabase::addApplicationFont(parser.value("font"));
        if (id >= 0)
            app.setFont(QFont(QFontDatabase::applicationFontFamilies(id).first(), 13));
    }
    theme(app, parser.isSet("dark"));
    QMainWindow window;
    window.setWindowTitle("Két Image Viewer · Gallery");
    auto *viewer = new ImageViewer(&window);
    window.setCentralWidget(viewer);
    auto *bar = window.addToolBar("Gallery");
    bar->setMovable(false);
    bar->setToolButtonStyle(Qt::ToolButtonTextOnly);
    auto openFiles = [viewer](const QStringList &paths) {
        QList<ImageItem> images;
        for (const auto &path : paths)
            images.append({QFileInfo(path).absoluteFilePath(), QFileInfo(path).fileName(), path, {}, {}, {}});
        viewer->setItems(images);
    };
    auto *open = bar->addAction("Open images…");
    open->setShortcut(QKeySequence::Open);
    QObject::connect(open, &QAction::triggered, &window, [&] {
        const auto paths = QFileDialog::getOpenFileNames(
            &window, "Open images", {},
            "Images (*.png *.jpg *.jpeg *.webp *.gif *.bmp *.tif *.tiff);;All files (*)");
        if (!paths.isEmpty())
            openFiles(paths);
    });
    QObject::connect(bar->addAction("Samples"), &QAction::triggered, viewer,
                     [viewer] { viewer->setItems(samples()); });
    auto *dark = bar->addAction("Dark");
    dark->setCheckable(true);
    dark->setChecked(parser.isSet("dark"));
    QObject::connect(dark, &QAction::toggled, &window, [&](bool enabled) { theme(app, enabled); });
    QObject::connect(bar->addAction("Fullscreen"), &QAction::triggered, &window,
                     [&] { window.isFullScreen() ? window.showNormal() : window.showFullScreen(); });
    QObject::connect(viewer, &ImageViewer::closeRequested, &window, &QWidget::close);
    QObject::connect(new QShortcut(QKeySequence(Qt::Key_Escape), &window), &QShortcut::activated, &window,
                     [&] {
                         if (window.isFullScreen())
                             window.showNormal();
                         else
                             window.close();
                     });
    QObject::connect(viewer, &ImageViewer::saveRequested, &window, [&] {
        const auto path = QFileDialog::getSaveFileName(&window, "Export displayed image", "image.png",
                                                       "PNG (*.png);;JPEG (*.jpg)");
        if (path.isEmpty())
            return;
        QString error;
        if (!viewer->saveCurrent(path, &error))
            QMessageBox::warning(&window, "Export failed", error);
    });
    auto status = [&] {
        window.statusBar()->showMessage(
            viewer->state() == ImageViewer::State::Ready
                ? "Scroll: zoom  ·  Drag: pan  ·  ← →: browse  ·  R: rotate  ·  F: fit  ·  0: 100%"
            : viewer->state() == ImageViewer::State::Error ? viewer->errorString()
                                                           : "Két Image Viewer · Qt Widgets · 0.1.0");
    };
    QObject::connect(viewer, &ImageViewer::stateChanged, &window, status);
    if (parser.positionalArguments().isEmpty())
        viewer->setItems(samples());
    else
        openFiles(parser.positionalArguments());
    window.resize(qBound(480, parser.value("width").toInt(), 2200), 800);
    window.show();
    viewer->canvas()->setFocus();
    if (parser.isSet("capture"))
        QTimer::singleShot(1200, &window,
                           [&] { app.exit(window.grab().save(parser.value("capture")) ? 0 : 2); });
    return app.exec();
}
