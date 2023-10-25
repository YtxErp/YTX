cmake_minimum_required(VERSION 3.19)
project(YTX VERSION 0.1.8 LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS LinguistTools Widgets Sql Gui Concurrent)
qt_standard_project_setup(REQUIRES 6.5)

add_definitions(-DQT_DISABLE_DEPRECATED_UP_TO=0x060500)

include(FetchContent)

FetchContent_Declare(
  YXlsx
  GIT_REPOSITORY https://github.com/YtxErp/YXlsx.git
  GIT_TAG        main
  SOURCE_SUBDIR  YXlsx
)
FetchContent_MakeAvailable(YXlsx)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wconversion -Wno-shorten-64-to-32")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

file(GLOB_RECURSE PROJECT_SOURCES
    "*.h"
    "*.cc"
    "*.ui"
  )

set(brc "${CMAKE_CURRENT_SOURCE_DIR}/resource/resource.brc")

if(APPLE)
  set(icon_mac "${CMAKE_CURRENT_SOURCE_DIR}/resource/logo/logo.icns")
  set_source_files_properties(${icon_mac} ${brc} PROPERTIES
    MACOSX_PACKAGE_LOCATION "Resources")

  qt6_add_executable(${PROJECT_NAME} MACOSX_BUNDLE ${PROJECT_SOURCES} ${icon_mac} ${brc})

  set_target_properties(${PROJECT_NAME} PROPERTIES
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist
  )
elseif(WIN32)
  set(INFO_RC "${CMAKE_CURRENT_SOURCE_DIR}/Info.rc")
  qt6_add_executable(${PROJECT_NAME} WIN32 ${PROJECT_SOURCES} ${INFO_RC} ${brc})
else()
  qt6_add_executable(${PROJECT_NAME} ${PROJECT_SOURCES} ${brc})
endif()

# Projects mode -> Build & Run -> Build -> Build Steps -> Details -> Targets, check "update_translations"
qt6_add_translations(${PROJECT_NAME}
  TS_FILES resource/I18N/ytx_zh_CN.ts)

target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::Widgets Qt6::Sql Qt6::Gui Qt6::Concurrent YXlsx::YXlsx)
