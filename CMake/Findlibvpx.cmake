# - Try to find libvpx.
# - Set VPX_ROOT on command line to provide an alternate location.
find_path(VPX_INCLUDE_DIR
  NAMES
    vpx/vpx_codec.h
  DOC
    "vpx include directory"
  HINTS
    "${VPX_ROOT}/include"
)
mark_as_advanced(VPX_INCLUDE_DIR)

if (WIN32)
  if (CMAKE_VS_PLATFORM_TOOLSET_HOST_ARCHITECTURE STREQUAL "x86")
    find_library(VPX_LIBRARY
    NAMES
      libvpx
    DOC
      "vpx library"
    HINTS
      "${VPX_ROOT}/lib"
      "${VPX_ROOT}/lib/x86"
    )
  elseif (CMAKE_VS_PLATFORM_TOOLSET_HOST_ARCHITECTURE STREQUAL "x64")
    find_library(VPX_LIBRARY
    NAMES
      libvpx
    DOC
      "vpx library"
    HINTS
      "${VPX_ROOT}/lib"
      "${VPX_ROOT}/lib/x64"
    )
  endif ()
else ()
  find_library(VPX_LIBRARY
  NAMES
    libvpx.a
  DOC
    "vpx library"
  HINTS
    "${VPX_ROOT}/lib"
    "${VPX_ROOT}/lib64"
  )
endif ()
mark_as_advanced(VPX_LIBRARY)

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(libvpx REQUIRED_VARS VPX_LIBRARY VPX_INCLUDE_DIR)

if (libvpx_FOUND)
  set(VPX_LIBRARIES "${VPX_LIBRARY}")
  set(VPX_INCLUDE_DIRS "${VPX_INCLUDE_DIR}")

  if (NOT TARGET libvpx::vpx)
    add_library(libvpx::vpx UNKNOWN IMPORTED)
    set_target_properties(libvpx::vpx PROPERTIES
      IMPORTED_LOCATION "${VPX_LIBRARY}"
      INTERFACE_INCLUDE_DIRECTORIES ${VPX_INCLUDE_DIR}
    )
  endif ()
endif ()
