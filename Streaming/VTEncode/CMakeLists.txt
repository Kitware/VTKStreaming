set(classes
  vtkVideoToolboxEncoder
)

vtk_module_add_module(VTKStreaming::VTEncode
  CLASSES ${classes}
)

if(APPLE)
  vtk_module_link(VTKStreaming::VTEncode
    PRIVATE
      "-framework VideoToolbox"
      "-framework CoreMedia"
      "-framework CoreVideo"
      "-framework CoreFoundation")
endif()
