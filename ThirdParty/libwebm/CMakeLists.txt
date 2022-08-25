unset(CMAKE_CXX_VISIBILITY_PRESET)
unset(CMAKE_VISIBILITY_INLINES_HIDDEN)

vtk_module_third_party(
  INTERNAL
    LICENSE_FILES "LICENSE.TXT;PATENTS.TXT"
    VERSION       "1.0.0.28"
    STANDARD_INCLUDE_DIRS
)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtk_libwebm.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtk_libwebm.h")

vtk_module_install_headers(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/vtk_libwebm.h")
