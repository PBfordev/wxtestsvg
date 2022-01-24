###############################################################################
## Name:        CMakeLists.txt
## Purpose:     To build wxTestSVG application
## Author:      PB
## Created:     2022-01-20
## Copyright:   (c) 2022 PB
## Licence:     wxWindows licence
###############################################################################

cmake_minimum_required(VERSION 3.1 FATAL_ERROR)
project(wxTestSVG)

find_package(wxWidgets 3.1.6 COMPONENTS webview core base REQUIRED)

set(SOURCES
  bmpbndl_svg_d2d.h
  bmpbndl_svg_d2d.cpp
  svgapp.cpp
  svgbench.h
  svgbench.cpp
  svgframe.h
  svgframe.cpp  
)

if (WIN32)
  list(APPEND SOURCES "${wxWidgets_ROOT_DIR}/include/wx/msw/wx.rc")
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property (DIRECTORY PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})

add_executable(${PROJECT_NAME} ${SOURCES})

include(${wxWidgets_USE_FILE})

set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED YES
)

if (WIN32)
  target_compile_definitions(${PROJECT_NAME} PRIVATE
    wxUSE_RC_MANIFEST
    wxUSE_DPI_AWARE_MANIFEST=2)
  set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE YES)

  if (MSVC)
     set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /MANIFEST:NO")
     set(EXTRA_WIN_LIBRARIES D2d1 windowscodecs)
  endif(MSVC)

if (WIN32)
  list(APPEND SOURCES "${wxWidgets_ROOT_DIR}/include/wx/msw/wx.rc")
endif()

endif()

target_link_libraries(${PROJECT_NAME} PRIVATE ${wxWidgets_LIBRARIES} ${EXTRA_WIN_LIBRARIES})