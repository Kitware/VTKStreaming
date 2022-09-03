set(classes 
  vtkAbstractVideoDecoder
  vtkAbstractVideoEncoder
  vtkCodedVideoPacket
  vtkRawVideoFrame
)

set(priv_classes 
  vtkAbstractDecoderDelegate
  vtkAbstractEncoderDelegate
  vtkSynchronousDecoderDelegate
  vtkSynchronousEncoderDelegate
  vtkAsynchronousDecoderDelegate
  vtkAsynchronousEncoderDelegate
)

set(headers
  vtkCodecTypes.h
  vtkPixelFormats.h
  vtkVideoProcessingStatusTypes.h
)

vtk_module_add_module(VTKVideo::Core 
  CLASSES ${classes}
  HEADERS ${headers}
  PRIVATE_CLASSES ${priv_classes}
)
