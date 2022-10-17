set(classes 
  vtkVideoEncoder
)

vtk_module_add_module(VTKStreaming::Encode
  CLASSES ${classes}
)
