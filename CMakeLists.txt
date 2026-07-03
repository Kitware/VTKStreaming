cmake_minimum_required(VERSION 3.27...4.0)
project(${SKBUILD_PROJECT_NAME} VERSION ${SKBUILD_PROJECT_VERSION})

option(VTKSTREAMING_ENABLE_TESTING "Enable testing." OFF)

set(VTKSTREAMING_USE_LIBVPX ON)
set(modules
  VTKStreaming::Core
  VTKStreaming::Encode
  VTKStreaming::Decode
  VTKStreaming::NvEncode
  VTKStreaming::libvpx
  VTKStreaming::VpxEncode
  VTKStreaming::VpxDecode
  VTKStreaming::OpenGL2
  VTKStreaming::WEBM
)

include(VTKSDKPythonWheelHelper)
vtksdk_build_modules(${SKBUILD_PROJECT_NAME} MODULES ${modules})
vtksdk_generate_package_init(${SKBUILD_PROJECT_NAME} MODULES ${modules})

# Copy the generated compile_commands.json into the source directory so that
# editors and language servers (e.g. clangd) can find it. The file is only
# produced by generators that support it (e.g. Ninja) when
# CMAKE_EXPORT_COMPILE_COMMANDS is ON.
if(CMAKE_EXPORT_COMPILE_COMMANDS)
  add_custom_target(copy_compile_commands ALL
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_BINARY_DIR}/compile_commands.json"
            "${CMAKE_SOURCE_DIR}/compile_commands.json"
    COMMENT "Copying compile_commands.json to source directory"
    VERBATIM)
endif()
