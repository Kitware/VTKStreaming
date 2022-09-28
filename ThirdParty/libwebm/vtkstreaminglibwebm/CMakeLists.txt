project(LIBWEBM CXX)

set(LIBWEBM_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

set(libwebm_common_headers
    "${LIBWEBM_SRC_DIR}/common/webmids.h"
    "${LIBWEBM_SRC_DIR}/common/vtk_libwebm_mangle.h")

set(mkvmuxer_sources
    "${LIBWEBM_SRC_DIR}/mkvmuxer/mkvmuxer.cc"
    "${LIBWEBM_SRC_DIR}/mkvmuxer/mkvmuxerutil.cc"
    "${LIBWEBM_SRC_DIR}/mkvmuxer/mkvwriter.cc")

set(mkvmuxer_headers
    "${LIBWEBM_SRC_DIR}/mkvmuxer/mkvmuxer.h"
    "${LIBWEBM_SRC_DIR}/mkvmuxer/mkvmuxertypes.h"
    "${LIBWEBM_SRC_DIR}/mkvmuxer/mkvmuxerutil.h"
    "${LIBWEBM_SRC_DIR}/mkvmuxer/mkvwriter.h")

set(mkvparser_sources
    "${LIBWEBM_SRC_DIR}/mkvparser/mkvparser.cc"
    "${LIBWEBM_SRC_DIR}/mkvparser/mkvreader.cc")

set(mkvparser_headers
    "${LIBWEBM_SRC_DIR}/mkvparser/mkvparser.h"
    "${LIBWEBM_SRC_DIR}/mkvparser/mkvreader.h")

vtk_module_add_module(VTKStreaming::libwebm
  SOURCES ${mkvmuxer_sources} ${mkvparser_sources}
  HEADERS ${libwebm_common_headers} ${mkvmuxer_headers} ${mkvparser_headers}
  HEADERS_SUBDIR "vtkstreaminglibwebm")

vtk_module_include(VTKStreaming::libwebm
  PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>")
