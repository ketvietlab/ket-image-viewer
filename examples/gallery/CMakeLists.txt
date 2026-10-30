qt_add_executable(ket_image_viewer_demo MACOSX_BUNDLE WIN32 main.cpp)
target_link_libraries(ket_image_viewer_demo PRIVATE Ket::ImageViewer)
set_target_properties(ket_image_viewer_demo PROPERTIES OUTPUT_NAME "Két Image Viewer"
    MACOSX_BUNDLE_BUNDLE_NAME "Két Image Viewer"
    MACOSX_BUNDLE_GUI_IDENTIFIER "vn.ket.imageviewer.demo"
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION})
