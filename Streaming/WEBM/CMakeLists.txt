set(classes 
  vtkWEBMMuxer
)

set(sources
  vtkMKVWriterImplementation.cxx
)

set(priv_headers
  vtkMKVWriterImplementation.h
)

vtk_module_add_module(VTKStreaming::WEBM 
  CLASSES ${classes} 
  PRIVATE_HEADERS ${priv_headers} 
  SOURCES ${sources})
