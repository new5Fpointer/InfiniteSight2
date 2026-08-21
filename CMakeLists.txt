cmake_minimum_required(VERSION 3.20)
project(InfiniteSight VERSION 1.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find Qt6 components
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets OpenGLWidgets)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Optional: Find libvips
find_package(VIPS QUIET)

set(SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/MainWindow.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/SettingsWindow.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/ThemeManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/ZoomableGraphicsView.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/manager/AppController.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/manager/ImageNavigator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/manager/LoadCoordinator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/manager/ViewStateManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/SettingsManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/ImageLoader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/ImageCache.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/Logger.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/ExifParser.cpp
)

set(HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/MainWindow.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/SettingsWindow.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/Theme.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/ThemeManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/ZoomableGraphicsView.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/manager/AppController.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/manager/ImageNavigator.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/manager/LoadCoordinator.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/manager/ViewStateManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/SettingsManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/ImageLoader.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/ImageCache.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/Logger.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/ExifParser.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/common/ImageModel.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/common/AppConstants.h
)

set(RESOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/resources/resources.qrc
)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS} ${RESOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::OpenGLWidgets
)

if(VIPS_FOUND)
    target_include_directories(${PROJECT_NAME} PRIVATE ${VIPS_INCLUDE_DIRS})
    target_link_libraries(${PROJECT_NAME} PRIVATE ${VIPS_LIBRARIES})
    target_compile_definitions(${PROJECT_NAME} PRIVATE ${VIPS_DEFINITIONS})
    message(STATUS "libvips found: ${VIPS_VERSION}")

    # Link libexif for ExifParser
    find_library(LIBEXIF NAMES exif libexif HINTS "${VIPS_ROOT}/lib" "$ENV{VIPS_ROOT}/lib" ENV VCPKG_ROOT/installed/x64-windows/lib)
    if(LIBEXIF)
        target_link_libraries(${PROJECT_NAME} PRIVATE ${LIBEXIF})
        get_filename_component(LIBEXIF_DIR "${LIBEXIF}" DIRECTORY)
        get_filename_component(LIBEXIF_ROOT "${LIBEXIF_DIR}" DIRECTORY)
        target_include_directories(${PROJECT_NAME} PRIVATE "${LIBEXIF_ROOT}/include")
    else()
        message(WARNING "libexif not found. EXIF parsing will fail.")
    endif()

    # Auto-copy vips DLLs to output directory on Windows
    if(WIN32)
        foreach(_lib IN LISTS VIPS_LIBRARIES)
            if(EXISTS "${_lib}")
                get_filename_component(_lib_dir "${_lib}" DIRECTORY)
                file(GLOB _vips_dlls "${_lib_dir}/../bin/*.dll")
                foreach(_dll IN LISTS _vips_dlls)
                    get_filename_component(_dll_name "${_dll}" NAME)
                    if(_dll_name MATCHES "vips|glib|gobject|gmodule|gio|gthread|intl|iconv|pcre|ffi|z|jpeg|png|tiff|webp|expat|harfbuzz|freetype|fontconfig|cairo|pixman|rsvg|xml|lzma|bz2|sqlite|crypto|ssl|curl|nghttp|brotli|exif|charset|croco|fftw|fribidi|gdk_pixbuf|girepository|lcms|sharpyuv|pango|pthread")
                        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                            "${_dll}"
                            $<TARGET_FILE_DIR:${PROJECT_NAME}>
                        )
                    endif()
                endforeach()
            endif()
        endforeach()
    endif()
else()
    message(WARNING "libvips not found. Large image support will be disabled. Set VIPS_ROOT to a vips-dev installation.")
endif()

# Platform-specific settings
if(WIN32)
    set_property(TARGET ${PROJECT_NAME} PROPERTY WIN32_EXECUTABLE TRUE)
endif()

# Install targets
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    BUNDLE DESTINATION .
)
