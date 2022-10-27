set(classes
  vtkVpxDecoder
)

vtk_module_add_module(VTKStreaming::VpxDecode
  CLASSES ${classes}
)
