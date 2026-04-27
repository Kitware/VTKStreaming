cmake_minimum_required(VERSION 3.27...4.0)
project(${SKBUILD_PROJECT_NAME} VERSION ${SKBUILD_PROJECT_VERSION})

option(VTKSTREAMING_ENABLE_TESTING "Enable testing." OFF)

set(VTKSTREAMING_USE_LIBVPX ON)
set(modules
  VTKStreaming::Core
  VTKStreaming::Encode
  VTKStreaming::NvEncode
  VTKStreaming::libvpx
  VTKStreaming::VpxEncode
  VTKStreaming::OpenGL2
  VTKStreaming::WEBM
)

include(VTKSDKPythonWheelHelper)
vtksdk_build_modules(${SKBUILD_PROJECT_NAME} MODULES ${modules})
vtksdk_generate_package_init(${SKBUILD_PROJECT_NAME} MODULES ${modules})
