set (classes
  vtkVpxEncoder
)

vtk_module_add_module(VTKStreaming::VpxEncode
  CLASSES ${classes}
)

if (VTKSTREAMING_USE_LIBVPX)
  vtk_module_link(VTKStreaming::VpxEncode
    PRIVATE
      VTKStreaming::libvpx
  )
  vtk_module_definitions(VTKStreaming::VpxEncode
    PRIVATE
      VTKSTREAMING_USE_LIBVPX
  )
endif ()
