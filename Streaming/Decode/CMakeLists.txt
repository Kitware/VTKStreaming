set(classes 
  vtkVideoDecoder
  vtkMockVideoDecoder
)

set(priv_classes
  vtkAsynchronousDecoderDelegate
)

vtk_module_add_module(VTKStreaming::Decode
  CLASSES ${classes}
  PRIVATE_CLASSES ${priv_classes}
)
