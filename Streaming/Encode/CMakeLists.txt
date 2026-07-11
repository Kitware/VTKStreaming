set(classes
  vtkVideoEncoder
  vtkEncoderFactory # TEMP: interim encoder selector; delete for VTK 9.7 (override attributes)
)

vtk_module_add_module(VTKStreaming::Encode
  CLASSES ${classes}
)
