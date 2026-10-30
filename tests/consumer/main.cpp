#include <QApplication>
#include <ket/imageviewer/image_viewer.h>
int main(int argc, char **argv) {
    QApplication app(argc, argv);
    ket::imageviewer::ImageViewer viewer;
    return viewer.count() == 0 && viewer.state() == ket::imageviewer::ImageViewer::State::Empty ? 0 : 1;
}
