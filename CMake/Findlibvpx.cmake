# - Try to find libvpx.
find_path(VPX_INCLUDE_DIR
  NAMES
    vpx/vpx_codec.h
  DOC
    "vpx include directory"
  PATHS
    "${VPX_ROOT}/include"
)
mark_as_advanced(VPX_INCLUDE_DIR)

find_library(VPX_LIBRARY
  NAMES
    vpx
  DOC
    "vpx library"
  PATHS
    "${VPX_ROOT}/lib"
    "${VPX_ROOT}/lib64"
)
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
