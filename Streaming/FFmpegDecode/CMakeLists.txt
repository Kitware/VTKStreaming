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
  vtkFFmpegSoftwareDecoder
)

set(private_classes
  vtkFFmpegDecoderInternals
)

vtk_module_add_module(VTKStreaming::FFmpegDecode CLASSES ${classes} PRIVATE_CLASSES ${private_classes})
vtk_module_link(VTKStreaming::FFmpegDecode
  PRIVATE
    FFMPEG::avformat
    FFMPEG::avcodec
    FFMPEG::avutil
    FFMPEG::swscale
)
