set(classes
  vtkOpenGLVideoFrame
)

set(priv_classes
  vtkOpenGLIYUVRenderDelegate
  vtkOpenGLNV12RenderDelegate
  vtkOpenGLRGB24RenderDelegate
  vtkOpenGLRGBA32RenderDelegate
)


set(shader_files
  glsl/vtkIYUVRenderFS.glsl
  glsl/vtkNV12RenderFS.glsl)

unset(shader_h_files)
foreach(file IN LISTS shader_files)
  vtk_encode_string(
    INPUT         "${file}"
    EXPORT_SYMBOL "VTKSTREAMINGOPENGL2_EXPORT"
    EXPORT_HEADER "vtkStreamingOpenGL2Module.h"
    HEADER_OUTPUT header
    SOURCE_OUTPUT source)
  list(APPEND sources
    "${source}")
  list(APPEND private_headers
    "${header}")
endforeach()

vtk_module_add_module(VTKStreaming::OpenGL2
  CLASSES ${classes}
  SOURCES ${sources}
  PRIVATE_CLASSES ${priv_classes}
  PRIVATE_HEADERS ${private_headers}
)
