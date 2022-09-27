set(classes 
  vtkNvEncoderGL
  vtkNvEncodeLoader
  vtkCUDADriverLoader
)

set(private_classes
  vtkNvEncoderInternals
)

set(headers
  vtkNvDynamicLoader.h
)

set(private_headers
  nvEncodeAPI.h
  vtkCUDADriverAPI.h
)

vtk_module_add_module(VTKStreaming::NvEncode
  CLASSES ${classes}
  HEADERS ${headers}
  PRIVATE_CLASSES ${private_classes}
  PRIVATE_HEADERS ${private_headers}
)
