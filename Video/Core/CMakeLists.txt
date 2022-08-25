set(classes 
  vtkAbstractVideoDecoder
  vtkAbstractVideoEncoder
  vtkCodedVideoPacket
  vtkRawVideoFrame
)

set(headers
  vtkCodecTypes.h
  vtkPixelFormats.h
)

vtk_module_add_module(VTKVideo::Core 
  CLASSES ${classes}
  HEADERS ${headers}
)
