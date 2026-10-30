cmake_minimum_required(VERSION 3.24)
project(KetImageViewer VERSION 0.1.0 LANGUAGES CXX)
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(GenerateExportHeader)
option(KET_IMAGE_VIEWER_BUILD_EXAMPLES "Build the native gallery demo" ${PROJECT_IS_TOP_LEVEL})
option(KET_IMAGE_VIEWER_BUILD_TESTS "Build focused library tests" ${PROJECT_IS_TOP_LEVEL})
find_package(Qt6 6.8 REQUIRED COMPONENTS Widgets Concurrent)
add_library(ket_image_viewer
    src/viewport.cpp src/image_canvas.cpp src/image_viewer.cpp src/decoder.cpp
    include/ket/imageviewer/image_canvas.h include/ket/imageviewer/image_viewer.h)
add_library(Ket::ImageViewer ALIAS ket_image_viewer)
set_target_properties(ket_image_viewer PROPERTIES AUTOMOC ON EXPORT_NAME ImageViewer
    VERSION ${PROJECT_VERSION} SOVERSION ${PROJECT_VERSION_MAJOR}
    CXX_VISIBILITY_PRESET hidden VISIBILITY_INLINES_HIDDEN YES)
target_compile_features(ket_image_viewer PUBLIC cxx_std_17)
generate_export_header(ket_image_viewer EXPORT_FILE_NAME include/ket/imageviewer/export.h
    EXPORT_MACRO_NAME KET_IMAGE_VIEWER_EXPORT STATIC_DEFINE KET_IMAGE_VIEWER_STATIC)
if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(ket_image_viewer PUBLIC KET_IMAGE_VIEWER_STATIC)
endif()
target_include_directories(ket_image_viewer PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}> PRIVATE src)
target_link_libraries(ket_image_viewer PUBLIC Qt6::Widgets PRIVATE Qt6::Concurrent)
target_compile_options(ket_image_viewer PRIVATE
    $<$<CXX_COMPILER_ID:Clang,AppleClang,GNU>:-Wall;-Wextra;-Wpedantic>
    $<$<CXX_COMPILER_ID:MSVC>:/W4;/utf-8>)
install(TARGETS ket_image_viewer EXPORT KetImageViewerTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/include/ket/imageviewer/export.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/ket/imageviewer)
install(EXPORT KetImageViewerTargets NAMESPACE Ket:: DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/KetImageViewer)
configure_package_config_file(cmake/KetImageViewerConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/KetImageViewerConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/KetImageViewer)
write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/KetImageViewerConfigVersion.cmake
    VERSION ${PROJECT_VERSION} COMPATIBILITY SameMinorVersion)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/KetImageViewerConfig.cmake ${CMAKE_CURRENT_BINARY_DIR}/KetImageViewerConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/KetImageViewer)
install(FILES LICENSE DESTINATION ${CMAKE_INSTALL_DATADIR}/licenses/KetImageViewer)
if(KET_IMAGE_VIEWER_BUILD_EXAMPLES)
    add_subdirectory(examples/gallery)
endif()
if(KET_IMAGE_VIEWER_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
