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

if (VTK_USE_CUDA)
  set(cuda_classes
    vtkNvEncoderCuda
  )
  set (cuda_headers
    #...
  )
endif ()

vtk_module_add_module(VTKStreaming::NVENC
  CLASSES ${classes} ${cuda_classes}
  PRIVATE_CLASSES ${private_classes}
  PRIVATE_HEADERS ${private_headers} ${cuda_headers}
)
