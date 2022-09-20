set(classes 
  vtkVideoEncoder
  vtkMockVideoEncoder
)

set(priv_classes
  vtkAsynchronousEncoderDelegate
)

vtk_module_add_module(VTKStreaming::Encode
  CLASSES ${classes}
  PRIVATE_CLASSES ${priv_classes}
)
