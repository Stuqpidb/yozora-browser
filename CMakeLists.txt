cmake_minimum_required(VERSION 3.21)

project(Yozora
    VERSION 0.2.0
    DESCRIPTION "Yozora Browser - a native desktop browser built with C++ and Qt WebEngine"
    HOMEPAGE_URL "https://github.com/Stuqpidb/yozora-browser"
    LANGUAGES CXX
)

# ---------------------------------------------------------------------------
# Qt
# ---------------------------------------------------------------------------
find_package(Qt6 6.8 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    WebEngineCore
    WebEngineWidgets
)

# Qt WebEngine is a Chromium embedding: it refuses to start if the sandbox
# bits are missing, and it needs very few exceptions for a desktop app.
set(CMAKE_AUTOMOC ON)

# ---------------------------------------------------------------------------
# Global settings
# ---------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Keep runtime artefacts next to each other for a simple, self-contained build.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
foreach(cfg IN ITEMS DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${cfg} "${CMAKE_BINARY_DIR}/bin")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${cfg} "${CMAKE_BINARY_DIR}/bin")
endforeach()

# ---------------------------------------------------------------------------
# Version header (single source of truth: CMake project VERSION)
# ---------------------------------------------------------------------------
set(YOZORA_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(YOZORA_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(YOZORA_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(YOZORA_VERSION_STRING "${PROJECT_VERSION}")
set(YOZORA_DISPLAY_NAME "Yozora Browser")
set(YOZORA_ORG_NAME "Yozora")
set(YOZORA_DOMAIN "yozora.org")
set(YOZORA_COPYRIGHT "Copyright (C) 2026 The Yozora Browser Authors")
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/src/utils/Version.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/generated/utils/Version.h"
    @ONLY
)

# ---------------------------------------------------------------------------
# Core library (all application code except main.cpp, so tests can link it)
# ---------------------------------------------------------------------------
add_library(yozora_core STATIC
    src/app/AppPaths.cpp
    src/app/AppPaths.h
    src/browser/BrowserTab.cpp
    src/browser/BrowserTab.h
    src/browser/BrowserWindow.cpp
    src/browser/BrowserWindow.h
    src/core/BookmarkStore.cpp
    src/core/BookmarkStore.h
    src/core/HistoryStore.cpp
    src/core/HistoryStore.h
    src/core/SearchEngine.cpp
    src/core/SearchEngine.h
    src/core/Settings.cpp
    src/core/Glass.cpp
    src/core/Glass.h
    src/core/Settings.h
    src/core/Theme.cpp
    src/core/Theme.h
    src/core/UpdateChecker.cpp
    src/core/UpdateChecker.h
    src/home/HomePage.cpp
    src/home/HomePage.h
    src/home/FlowLayout.cpp
    src/home/FlowLayout.h
    src/home/HomeWidget.cpp
    src/home/HomeWidget.h
    src/home/HomeWidgets.cpp
    src/home/HomeWidgets.h
    src/privacy/DownloadSafety.cpp
    src/privacy/DownloadSafety.h
    src/privacy/PermissionManager.cpp
    src/privacy/PermissionManager.h
    src/privacy/RequestInterceptor.cpp
    src/privacy/RequestInterceptor.h
    src/privacy/TrackerList.cpp
    src/privacy/TrackerList.h
    src/ui/AddressBar.cpp
    src/ui/AddressBar.h
    src/ui/ClearBrowsingDataDialog.cpp
    src/ui/ClearBrowsingDataDialog.h
    src/ui/NavigationBar.cpp
    src/ui/NavigationBar.h
    src/ui/PermissionPrompt.cpp
    src/ui/PermissionPrompt.h
    src/ui/RailButton.cpp
    src/ui/RailButton.h
    src/ui/SettingsDialog.cpp
    src/ui/SettingsDialog.h
    src/ui/SideBar.cpp
    src/ui/SideBar.h
    src/ui/TabStrip.cpp
    src/ui/TabStrip.h
    src/ui/TabWidget.cpp
    src/ui/TabWidget.h
    src/utils/UrlUtils.cpp
    src/utils/UrlUtils.h
    src/web/ErrorPage.cpp
    src/web/ErrorPage.h
    src/web/WebPage.cpp
    src/web/WebPage.h
    src/web/WebProfile.cpp
    src/web/WebProfile.h
    src/web/WebView.cpp
    src/web/WebView.h
    src/web/DownloadManager.cpp
    src/web/DownloadManager.h
    resources/resources.qrc
)

target_include_directories(yozora_core
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
        "${CMAKE_CURRENT_BINARY_DIR}/generated"
)

target_link_libraries(yozora_core
    PUBLIC
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::WebEngineWidgets
        Qt6::WebEngineCore
)

if(MSVC)
    target_compile_options(yozora_core PRIVATE /W4 /permissive- /utf-8 /Zc:__cplusplus)
    target_compile_definitions(yozora_core PRIVATE
        NOMINMAX
        WIN32_LEAN_AND_MEAN
        _WIN32_WINNT=0x0A00
    )
    # DwmSetWindowAttribute backs the native window material (Mica / Acrylic).
    target_link_libraries(yozora_core PRIVATE dwmapi)
else()
    target_compile_options(yozora_core PRIVATE -Wall -Wextra)
endif()

# ---------------------------------------------------------------------------
# Executable
# ---------------------------------------------------------------------------
qt_add_executable(yozora
    WIN32
    src/main.cpp
)

target_link_libraries(yozora PRIVATE yozora_core)

set_target_properties(yozora PROPERTIES
    OUTPUT_NAME "Yozora"
    WIN32_EXECUTABLE ON
    MACOSX_BUNDLE ON
    MACOSX_BUNDLE_BUNDLE_NAME "Yozora Browser"
    MACOSX_BUNDLE_GUI_IDENTIFIER "org.yozora.browser"
)

# Icon and version information in the executable itself.
if(WIN32)
    target_sources(yozora PRIVATE installer/yozora.rc)
endif()

# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------
include(CTest)
option(YOZORA_BUILD_TESTS "Build Yozora unit tests" ON)

if(YOZORA_BUILD_TESTS AND BUILD_TESTING)
    find_package(Qt6 6.5 REQUIRED COMPONENTS Test)
    add_subdirectory(tests)
endif()

# ---------------------------------------------------------------------------
# Install rules (see cmake/Packaging.cmake for the staging + installer logic)
# ---------------------------------------------------------------------------
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/Packaging.cmake")
