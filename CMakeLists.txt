cmake_minimum_required(VERSION 3.27...4.0)

# Two build paths:
# 1. VTKSTREAMING_USE_VTKSDK (default ON) - uses vtk-sdk-python-wheel-helper,
#    for building standalone wheels via scikit-build-core.
# 2. VTKSTREAMING_USE_VTKSDK OFF - uses standard VTK module system and
#    vtk_module_wrap_python(), for building within a superbuild (e.g., ParaView
#    superbuild) where VTK is provided as a findable CMake package.
option(VTKSTREAMING_USE_VTKSDK "Use VTK-SDK Python wheel helper for building" ON)

if(VTKSTREAMING_USE_VTKSDK)
  project(${SKBUILD_PROJECT_NAME} VERSION ${SKBUILD_PROJECT_VERSION})
else()
  project(vtkstreaming VERSION 0.4.0)
endif()

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

# VideoToolbox is an Apple-only system framework; only build the hardware
# encoder backend that uses it on macOS. Non-Apple builds are unaffected.
if(APPLE)
  list(APPEND modules VTKStreaming::VTEncode)
endif()

if(VTKSTREAMING_USE_VTKSDK)
  # --- Path 1: VTK-SDK wheel build (scikit-build-core) ---
  include(VTKSDKPythonWheelHelper)
  vtksdk_build_modules(${SKBUILD_PROJECT_NAME} MODULES ${modules})
  vtksdk_generate_package_init(${SKBUILD_PROJECT_NAME} MODULES ${modules})
else()
  # --- Path 2: Standard VTK module system (superbuild / native build) ---
  # VTK-targets.cmake unconditionally declares VTK::zlib's link interface as
  # ZLIB::ZLIB, but vtk-config.cmake only calls find_package(ZLIB) for the
  # third-party deps of the COMPONENTS actually requested below (CommonCore,
  # Python don't need zlib). At generate time CMake still validates every
  # target's link interface, including VTK::zlib, so ZLIB::ZLIB must already
  # exist. Find it explicitly first to satisfy that validation regardless of
  # which components are requested.
  find_package(ZLIB REQUIRED)
  find_package(VTK CONFIG REQUIRED COMPONENTS CommonCore Python)

  include(GNUInstallDirs)

  add_library(vtkstreaming_utility_target INTERFACE)

  vtk_module_find_modules(module_files ${CMAKE_SOURCE_DIR})

  vtk_module_scan(
    MODULE_FILES     ${module_files}
    REQUEST_MODULES  ${modules}
    PROVIDES_MODULES modules
    ENABLE_TESTS     OFF
  )

  vtk_module_python_default_destination(python_destination)
  vtk_module_build(
    MODULES               ${modules}
    ARCHIVE_DESTINATION   "${CMAKE_INSTALL_LIBDIR}"
    INSTALL_HEADERS       ON
    HEADERS_DESTINATION   "${CMAKE_INSTALL_INCLUDEDIR}"
    LICENSE_DESTINATION   "${CMAKE_INSTALL_LIBDIR}"
    HIERARCHY_DESTINATION "${CMAKE_INSTALL_LIBDIR}/hierarchy"
    RUNTIME_DESTINATION   "${CMAKE_INSTALL_LIBDIR}"
    LIBRARY_DESTINATION   "${CMAKE_INSTALL_LIBDIR}"
    UTILITY_TARGET        vtkstreaming_utility_target
  )

  vtk_module_wrap_python(
    MODULES         ${modules}
    SOABI           "${Python${VTK_PYTHON_VERSION}_SOABI}"
    PYTHON_PACKAGE  "vtk_streaming"
    MODULE_DESTINATION  "${python_destination}"
    LIBRARY_DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    UTILITY_TARGET      vtkstreaming_utility_target
    # Static Python modules are almost never wanted.
    BUILD_STATIC    OFF
    BUILD_PYI_FILES OFF
    INSTALL_HEADERS OFF
  )
  # Create an `__init__.py` containing wrapped filters.
  set(python_modules)
  foreach(module ${modules})
    _vtk_module_get_module_property("${module}"
      PROPERTY  "library_name"
      VARIABLE  library_name)
    list(APPEND python_modules "'${library_name}'")
  endforeach()

  list(JOIN python_modules ,  python_modules_string)
  configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/__init__.py.in"
    "${CMAKE_BINARY_DIR}/${python_destination}/vtk_streaming/__init__.py"
    @ONLY)
  install(
    FILES "${CMAKE_BINARY_DIR}/${python_destination}/vtk_streaming/__init__.py"
    DESTINATION "${python_destination}/vtk_streaming/")
endif()

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
