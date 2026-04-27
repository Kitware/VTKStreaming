cmake_minimum_required(VERSION 3.27...4.0)
project(${SKBUILD_PROJECT_NAME} VERSION ${SKBUILD_PROJECT_VERSION})

option(VTKSTREAMING_ENABLE_TESTING "Enable testing." OFF)

set(modules
  VTKStreaming::Core
  VTKStreaming::Decode
  VTKStreaming::Encode
  VTKStreaming::NvEncode
  VTKStreaming::OpenGL2
)

include(VTKSDKPythonWheelHelper)
vtksdk_build_modules(${SKBUILD_PROJECT_NAME} MODULES ${modules})
vtksdk_generate_package_init(${SKBUILD_PROJECT_NAME} MODULES ${modules})
