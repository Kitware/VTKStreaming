vtk_module_third_party(
  INTERNAL
    LICENSE_FILES "vtkstreaminglibvpx/LICENSE" "vtkstreaminglibvpx/PATENTS"
    VERSION       "1.0.0.28"
    STANDARD_INCLUDE_DIRS
  EXTERNAL
    PACKAGE libvpx
    TARGETS libvpx::vpx
    STANDARD_INCLUDE_DIRS
)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtkstreaming_libvpx.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtkstreaming_libvpx.h")

vtk_module_install_headers(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/vtkstreaming_libvpx.h")
