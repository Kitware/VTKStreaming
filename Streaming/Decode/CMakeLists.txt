set(classes 
  vtkVideoDecoder
)

vtk_module_add_module(VTKStreaming::Decode
  CLASSES ${classes}
)
