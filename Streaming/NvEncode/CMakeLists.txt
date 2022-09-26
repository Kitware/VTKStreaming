set(classes 
  vtkNvEncoderGL
  vtkNvEncodeImportTable
)

set(private_classes
  vtkNvEncoderInternals
)

set(private_headers
  nvEncodeAPI.h
)

vtk_module_add_module(VTKStreaming::NvEncode
  CLASSES ${classes}
  PRIVATE_CLASSES ${private_classes}
  PRIVATE_HEADERS ${private_headers}
)
