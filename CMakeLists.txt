cmake_minimum_required(VERSION 3.16)

project(zx4pixelizer VERSION 0.1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network)
find_package(OpenMP)

# ==============================================================================
# 1. Сборка подпроекта apng (zlib + libpng-apng + плагин qapng) без правок в apng/
# ==============================================================================
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/apng/src/plugins/imageformats/apng/apngimageplugin.cpp")
    message(STATUS "Found qapng submodule: building qapng imageformat plugin for Qt6")

    set(ZLIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/apng/src/3rdparty/zlib/src")
    set(LIBPNG_DIR "${CMAKE_CURRENT_SOURCE_DIR}/apng/src/3rdparty/libpng/src")
    set(QAPNG_DIR "${CMAKE_CURRENT_SOURCE_DIR}/apng/src/plugins/imageformats/apng")

    # Статическая библиотека zlib
    add_library(qapng_zlib STATIC
        "${ZLIB_DIR}/adler32.c"
        "${ZLIB_DIR}/compress.c"
        "${ZLIB_DIR}/crc32.c"
        "${ZLIB_DIR}/deflate.c"
        "${ZLIB_DIR}/gzclose.c"
        "${ZLIB_DIR}/gzlib.c"
        "${ZLIB_DIR}/gzread.c"
        "${ZLIB_DIR}/gzwrite.c"
        "${ZLIB_DIR}/infback.c"
        "${ZLIB_DIR}/inffast.c"
        "${ZLIB_DIR}/inflate.c"
        "${ZLIB_DIR}/inftrees.c"
        "${ZLIB_DIR}/trees.c"
        "${ZLIB_DIR}/uncompr.c"
        "${ZLIB_DIR}/zutil.c"
    )
    set_target_properties(qapng_zlib PROPERTIES POSITION_INDEPENDENT_CODE ON)
    target_include_directories(qapng_zlib PUBLIC "${ZLIB_DIR}")

    # Статическая библиотека libpng (с поддержкой APNG)
    add_library(qapng_libpng STATIC
        "${LIBPNG_DIR}/png.c"
        "${LIBPNG_DIR}/pngerror.c"
        "${LIBPNG_DIR}/pngget.c"
        "${LIBPNG_DIR}/pngmem.c"
        "${LIBPNG_DIR}/pngpread.c"
        "${LIBPNG_DIR}/pngread.c"
        "${LIBPNG_DIR}/pngrio.c"
        "${LIBPNG_DIR}/pngrtran.c"
        "${LIBPNG_DIR}/pngrutil.c"
        "${LIBPNG_DIR}/pngset.c"
        "${LIBPNG_DIR}/pngtrans.c"
        "${LIBPNG_DIR}/pngwio.c"
        "${LIBPNG_DIR}/pngwrite.c"
        "${LIBPNG_DIR}/pngwtran.c"
        "${LIBPNG_DIR}/pngwutil.c"
    )
    set_target_properties(qapng_libpng PROPERTIES POSITION_INDEPENDENT_CODE ON)
    target_compile_definitions(qapng_libpng PUBLIC
        PNG_APNG_SUPPORTED
        PNG_ARM_NEON_OPT=0
        PNG_INTEL_SSE_OPT=0
        PNG_MIPS_MSA_OPT=0
        PNG_POWERPC_VSX_OPT=0
    )
    target_include_directories(qapng_libpng PUBLIC "${LIBPNG_DIR}" "${ZLIB_DIR}")
    target_link_libraries(qapng_libpng PUBLIC qapng_zlib)

    # Динамический плагин imageformats/qapng
    add_library(qapng MODULE
        "${QAPNG_DIR}/apngimagehandler.cpp"
        "${QAPNG_DIR}/apngimagehandler_p.h"
        "${QAPNG_DIR}/apngimageplugin.cpp"
        "${QAPNG_DIR}/apngimageplugin.h"
        "${QAPNG_DIR}/apngreader.cpp"
        "${QAPNG_DIR}/apngreader_p.h"
        "${QAPNG_DIR}/apngtypes.cpp"
        "${QAPNG_DIR}/apngtypes_p.h"
        "${QAPNG_DIR}/apngwriter.cpp"
        "${QAPNG_DIR}/apngwriter_p.h"
    )
    target_compile_definitions(qapng PRIVATE QT_PLUGIN)
    target_include_directories(qapng PRIVATE "${QAPNG_DIR}" "${LIBPNG_DIR}" "${ZLIB_DIR}")
    target_link_libraries(qapng PRIVATE Qt6::Core Qt6::Gui qapng_libpng qapng_zlib)

    # Кладем собранный плагин сразу в bin/imageformats/, где его автоматически найдет Qt6
    set_target_properties(qapng PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/imageformats"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/imageformats"
    )
endif()

# ==============================================================================
# 2. Основное приложение zx4pixelizer
# ==============================================================================
set(PROJECT_SOURCES
    cie/CIEDE2000.cpp
    cie/CIEDE2000.h
    cie/color_conversions.cpp
    cie/color_conversions.h
    lz4/lz4.c
    lz4/lz4.h
    lz4/lz4hc.c
    lz4/lz4hc.h
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    mainwindow_events.cpp
    mainwindow_private.cpp
    scenewindow.cpp
    scenewindow.h
    scenewindow.ui
    spritewindow.cpp
    spritewindow.h
    spritewindow.ui
    zx4pixelizer.cpp
    zx4pixelizer.h
    resource.qrc
)

if(WIN32)
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/app_icon.rc"
         "IDI_ICON1 ICON DISCARDABLE \"${CMAKE_CURRENT_SOURCE_DIR}/resources/sonic.zx4.ico\"\n")
    list(APPEND PROJECT_SOURCES "${CMAKE_CURRENT_BINARY_DIR}/app_icon.rc")
endif()

qt_add_executable(zx4pixelizer WIN32 ${PROJECT_SOURCES})

if(TARGET qapng)
    add_dependencies(zx4pixelizer qapng)
endif()

target_compile_definitions(zx4pixelizer PRIVATE
    APP_VERSION="${PROJECT_VERSION}"
)

target_include_directories(zx4pixelizer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/cie
    ${CMAKE_CURRENT_SOURCE_DIR}/lz4
)

target_link_libraries(zx4pixelizer PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
)

if(OpenMP_CXX_FOUND)
    target_link_libraries(zx4pixelizer PRIVATE OpenMP::OpenMP_CXX)
endif()

set_target_properties(zx4pixelizer PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
