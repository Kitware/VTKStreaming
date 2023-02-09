set(classes
  vtkVpxDecoder
)

vtk_module_add_module(VTKStreaming::VpxDecode
  CLASSES ${classes}
)

if (VTKSTREAMING_USE_LIBVPX)
  vtk_module_link(VTKStreaming::VpxDecode
    PRIVATE
      VTKStreaming::libvpx
  )
  vtk_module_definitions(VTKStreaming::VpxDecode
    PRIVATE
      VTKSTREAMING_USE_LIBVPX
  )
endif ()
