set(classes 
  vtkCompressedVideoPacket
  vtkRawVideoFrame
)

set(headers
  vtkVideoCodecTypes.h
  vtkPixelFormatTypes.h
  vtkVideoProcessingStatusTypes.h
)

set(sources
  vtkVideoCodecTypes.cxx
  vtkPixelFormatTypes.cxx
  vtkVideoProcessingStatusTypes.cxx
)

vtk_module_add_module(VTKStreaming::Core 
  CLASSES ${classes}
  HEADERS ${headers}
  SOURCES ${sources}
)
