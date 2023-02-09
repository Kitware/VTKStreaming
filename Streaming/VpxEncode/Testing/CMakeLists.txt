vtk_module_test_data(
  cars_320x240.iyuv)

if (VTKSTREAMING_USE_LIBVPX)
  add_subdirectory(Cxx)
endif()
