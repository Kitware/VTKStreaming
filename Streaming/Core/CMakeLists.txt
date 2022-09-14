set(classes 
  vtkCompressedVideoPacket
  vtkRawVideoFrame
  vtkCPUVideoFrame
)

set(headers
  vtkVideoCodecTypes.h
  vtkPixelFormatTypes.h
  vtkVideoProcessingStatusTypes.h
)

vtk_module_add_module(VTKStreaming::Core 
  CLASSES ${classes}
  HEADERS ${headers}
)
