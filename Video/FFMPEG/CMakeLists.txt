vtk_module_find_package(
  PACKAGE FFMPEG
  VERSION 4.3.0
  FORWARD_VERSION_REQ MINOR
  COMPONENTS
    avformat
    avcodec
    avutil
    swscale
  OPTIONAL_COMPONENTS
    swresample)

set(classes 
  vtkFFMPEGSoftwareDecoder
  vtkFFMPEGSoftwareEncoder
  vtkFFMPEGVaapiEncoder
)

set(private_classes
  vtkFFMPEGDecoderInternals 
  vtkFFMPEGEncoderInternals  
)

vtk_module_add_module(VTKVideo::FFMPEG CLASSES ${classes} PRIVATE_CLASSES ${private_classes})
vtk_module_link(VTKVideo::FFMPEG
  PRIVATE
    FFMPEG::avformat
    FFMPEG::avcodec
    FFMPEG::avutil
    FFMPEG::swscale
)
