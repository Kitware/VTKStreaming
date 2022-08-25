set(classes 
  vtkWEBMWriter
)

set(sources
  vtkMKVWriterImplementation.cxx
)

set(priv_headers
  vtkMKVWriterImplementation.h
)

vtk_module_add_module(VTK::IOWEBM 
  CLASSES ${classes} 
  PRIVATE_HEADERS ${priv_headers} 
  SOURCES ${sources})

vtk_module_link(VTK::IOWEBM
  PRIVATE
    VTK::libwebm)
