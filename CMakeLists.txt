cmake_minimum_required(VERSION 3.19)
project(YTX VERSION 0.2.2 LANGUAGES CXX)

# Enable automatic processing of moc
set(CMAKE_AUTOMOC ON)

# Required Qt6 modules
find_package(Qt6 REQUIRED COMPONENTS LinguistTools Widgets Sql Gui Concurrent PrintSupport Network)

if(WIN32)
    set(OPENSSL_ROOT_DIR "D:/msys64/mingw64")
endif()

find_package(OpenSSL REQUIRED)

# Standard Qt project setup (Qt 6.5 or higher)
qt_standard_project_setup(REQUIRES 6.5)

# Disable deprecated Qt features up to 6.5
add_definitions(-DQT_DISABLE_DEPRECATED_UP_TO=0x060500)

# C++ standard settings
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wconversion -Wno-shorten-64-to-32")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Optionally enable debug build and flags
# set(CMAKE_BUILD_TYPE Debug)
# set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g")

# Add current directory to include path
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
add_subdirectory(external/YXlsx/YXlsx/)

# Collect all source files recursively
file(GLOB_RECURSE PROJECT_SOURCES
  "*.h"
  "*.cc"
  "*.ui"
)

# Resource file (Windows/macOS)
set(brc "${CMAKE_CURRENT_SOURCE_DIR}/resource/resource.brc")

# macOS-specific setup
if(APPLE)
  # macOS app icon
  set(icon_mac "${CMAKE_CURRENT_SOURCE_DIR}/resource/logo/logo.icns")

  # Place icon and resource in bundle Resources folder
  set_source_files_properties(${icon_mac} ${brc} PROPERTIES
    MACOSX_PACKAGE_LOCATION "Resources")

  # Build as macOS bundle
  qt6_add_executable(${PROJECT_NAME} MACOSX_BUNDLE
    ${PROJECT_SOURCES} ${icon_mac} ${brc})

  configure_file(
      "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist"
      "${CMAKE_CURRENT_BINARY_DIR}/Info.plist"
      @ONLY
  )

  # Bundle properties
  set_target_properties(${PROJECT_NAME} PROPERTIES
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_BINARY_DIR}/Info.plist
  )

  # Post-build: copy "print" folder into .app bundle's Resources
  add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E rm -rf "$<TARGET_BUNDLE_DIR:${PROJECT_NAME}>/Contents/Resources/print"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/print"
            "$<TARGET_BUNDLE_DIR:${PROJECT_NAME}>/Contents/Resources/print"
  )

# Windows-specific setup
elseif(WIN32)
  set(WINRC "C:\\Program Files (x86)\\Windows Kits\\10\\bin\\10.0.26100.0\\x86\\rc.exe")

  configure_file(
      "${CMAKE_CURRENT_SOURCE_DIR}/Info.rc"
      "${CMAKE_CURRENT_BINARY_DIR}/Info.rc"
      @ONLY
  )

  set(INFO_RC "${CMAKE_CURRENT_BINARY_DIR}/Info.rc")
  qt6_add_executable(${PROJECT_NAME} WIN32 ${PROJECT_SOURCES} ${INFO_RC} ${brc})

# Linux or other platforms
else()
  qt6_add_executable(${PROJECT_NAME} ${PROJECT_SOURCES} ${brc})
endif()

# Translation file (.ts -> .qm)
# Projects mode -> Build & Run -> Build -> Build Steps -> Details -> Targets, check "update_translations"
qt6_add_translations(${PROJECT_NAME}
  TS_FILES resource/I18N/ytx_zh_CN.ts)

# Link required libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
  Qt6::Widgets
  Qt6::Sql
  Qt6::Gui
  Qt6::Concurrent
  Qt6::PrintSupport
  Qt6::Network
  YXlsx::YXlsx
  OpenSSL::SSL
  OpenSSL::Crypto
)
