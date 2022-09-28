set(classes
  vtkOpenGLVideoFrame
)

set(priv_classes
  vtkOpenGLIYUVCaptureDelegate
  vtkOpenGLNV12CaptureDelegate
  vtkOpenGLIYUVRenderDelegate
  vtkOpenGLNV12RenderDelegate
  vtkOpenGLRGB24RenderDelegate
  vtkOpenGLRGBA32RenderDelegate
)

set(priv_headers
  vtkOpenGLVideoFrameInternals
)

set(shader_files
  glsl/vtkIYUVCaptureFS.glsl
  glsl/vtkNV12CaptureFS.glsl
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
