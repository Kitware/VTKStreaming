vtk_module_find_package(
  PACKAGE FFMPEG
  VERSION 4.0.0
  FORWARD_VERSION_REQ MINOR
  COMPONENTS
    avformat
    avcodec
    avutil
    swscale
  OPTIONAL_COMPONENTS
    swresample)

set(classes 
  vtkFFmpegSoftwareEncoder
  vtkFFmpegHardwareEncoder
)

set(private_classes
  vtkFFmpegEncoderInternals
)

vtk_module_add_module(VTKStreaming::FFmpegEncode CLASSES ${classes} PRIVATE_CLASSES ${private_classes})
vtk_module_link(VTKStreaming::FFmpegEncode
  PRIVATE
    FFMPEG::avformat
    FFMPEG::avcodec
    FFMPEG::avutil
    FFMPEG::swscale
)
