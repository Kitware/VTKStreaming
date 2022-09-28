cmake_minimum_required(VERSION 3.12...3.16 FATAL_ERROR)

project(VTKStreaming)

# Set a default build type if none was specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to 'Release' as none was specified.")
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release"
    "MinSizeRel" "RelWithDebInfo")
endif()

option(VTKSTREAMING_ENABLE_TESTING "Enable testing." ON)
option(VTKSTREAMING_BUILD_SHARED_LIBS "Generate shared libraries instead of static ones" ON)

include(GNUInstallDirs)

find_package(VTK COMPONENTS
  CommonCore
  RenderingOpenGL2
  TestingCore
)
# to use FindFFMPEG.cmake from VTK.
set(CMAKE_MODULE_PATH "${VTK_PREFIX_PATH}/lib/cmake/vtk-${VTK_MAJOR_VERSION}.${VTK_MINOR_VERSION}")
if (VTK_FOUND)
  message (STATUS "VTK (${VTK_VERSION}): ${VTK_PREFIX_PATH}")
endif ()

set(BUILD_SHARED_LIBS ${VTKSTREAMING_BUILD_SHARED_LIBS})
if (VTKSTREAMING_ENABLE_TESTING)
  include(CTest)
  set_property(CACHE BUILD_TESTING PROPERTY TYPE INTERNAL)
  set(BUILD_TESTING ON)
  message(STATUS "Tests will be built.")
else ()
  message(STATUS "Tests will not be built.")
  set(BUILD_TESTING OFF)
endif ()
set(VTKSTREAMING_INSTALL_EXPORT VTKSTREAMING)

if (NOT DEFINED VTKSTREAMING_CMAKE_DESTINATION)
  set(VTKSTREAMING_CMAKE_DESTINATION
    "${CMAKE_INSTALL_LIBDIR}/cmake/vtkstreaming")
endif ()

set(vtkstreaming_src_directories "${PROJECT_SOURCE_DIR}/Streaming" "${PROJECT_SOURCE_DIR}/ThirdParty")

set(_vtk_module_log ALL)
vtk_module_find_modules(VTKSTREAMING_MODULE_FILES ${vtkstreaming_src_directories})
vtk_module_find_kits(VTKSTREAMING_KIT_FILES ${vtkstreaming_src_directories})
vtk_module_scan(
  MODULE_FILES           ${VTKSTREAMING_MODULE_FILES}
  KIT_FILES           	 ${VTKSTREAMING_KIT_FILES}
  PROVIDES_MODULES       VTKSTREAMING_MODULES
  PROVIDES_KITS          VTKSTREAMING_KITS
  REQUIRES_MODULES       vtkstreaming_required_modules
  UNRECOGNIZED_MODULES 	 vtkstreaming_unrecognized_modules
  ENABLE_TESTS          "${VTKSTREAMING_ENABLE_TESTING}"
  WANT_BY_DEFAULT        ON
)
vtk_module_build(
  MODULES	               ${VTKSTREAMING_MODULES}
  INSTALL_EXPORT           ${VTKSTREAMING_INSTALL_EXPORT}
  INSTALL_HEADERS          ON
  HEADERS_DESTINATION     "${CMAKE_INSTALL_INCLUDEDIR}/vtkstreaming"
  CMAKE_DESTINATION       "${VTKSTREAMING_CMAKE_DESTINATION}"
  VERSION                 "${PROJECT_VERSION}"
  SOVERSION               "${PROJECT_VERSION}"
  USE_EXTERNAL            OFF
  TEST_INPUT_DATA_DIRECTORY   "${CMAKE_CURRENT_SOURCE_DIR}/Data"
  TEST_OUTPUT_DATA_DIRECTORY  "${CMAKE_CURRENT_SOURCE_DIR}/Data"
)
