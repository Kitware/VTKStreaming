enable_language(ASM_NASM)

set(VPX_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")
set(VPX_CONFIG_DIR "${CMAKE_CURRENT_BINARY_DIR}")
# x86/x86_64 assembly/intrinsics flags.
set(ENABLE_MMX ON)
set(ENABLE_SSE ON)
set(ENABLE_SSE2 ON)
set(ENABLE_SSE3 ON)
set(ENABLE_SSSE3 ON)
set(ENABLE_SSE4_1 ON)
set(ENABLE_AVX ON)
set(ENABLE_AVX2 ON)
set(ENABLE_AVX512 OFF)
set(CONFIG_ENCODERS ON)
set(CONFIG_DECODERS ON)
set(CONFIG_VP9 ON)
set(CONFIG_VP9_ENCODER ON)
set(CONFIG_VP9_DECODER ON)
set(CONFIG_REALTIME_ONLY ON)
set(ENABLE_NEON ON)
# For arm64 on apple systems, turn off runtime cpu detection.
# it lets neon intrinsics be compiled instead of neon assembly
if (APPLE AND CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "^arm")
  set(CONFIG_RUNTIME_CPU_DETECT OFF)
endif ()
set(LIBVPX_MANGLE_PREFIX "vtkstreaming_")

find_package(Git QUIET)
include("${VPX_ROOT}/build/cmake/vpx_determine_version.cmake")
determine_version("${VPX_ROOT}" ${GIT_EXECUTABLE} "VPX")
# configure vpx_version.h
configure_file("${VPX_ROOT}/vpx_version.h.in" "${VPX_CONFIG_DIR}/vpx_version.h" @ONLY)
# configure vpx_mangle.h
configure_file("${VPX_ROOT}/vpx_mangle.h.in" "${VPX_CONFIG_DIR}/vpx_mangle.h" @ONLY)

include ("${VPX_ROOT}/build/cmake/vpx_optimizations.cmake")
include ("${VPX_ROOT}/build/cmake/vpx_cpu.cmake")
include ("${VPX_ROOT}/build/cmake/vpx_os.cmake")
include ("${VPX_ROOT}/build/cmake/rtcd.cmake")
# include ("${VPX_ROOT}/build/cmake/vpx_options.cmake")
include ("${VPX_ROOT}/build/cmake/vpx_generate_config.cmake")
# set(VPX_CONFIG_ARGS "todo: cmake config")
# configure vpx_codec_config.c
configure_file("${VPX_ROOT}/vpx_codec_config.c.in" "${VPX_CONFIG_DIR}/vpx_codec_config.c" @ONLY)

add_subdirectory("${VPX_ROOT}/vpx")
add_subdirectory("${VPX_ROOT}/vpx_mem")
add_subdirectory("${VPX_ROOT}/vpx_ports")
add_subdirectory("${VPX_ROOT}/vpx_util")
add_subdirectory("${VPX_ROOT}/vpx_scale")
add_subdirectory("${VPX_ROOT}/vpx_dsp")
add_subdirectory("${VPX_ROOT}/vp8")
add_subdirectory("${VPX_ROOT}/vp9")

vtk_module_add_module(VTKStreaming::libvpx
SOURCES ${vpx_installable_headers}
)

list(APPEND _vpx_c_targets
  "vpx_codec"
  "vpx_mem"
  "vpx_util"
)

list(APPEND _vpx_targets_with_asm_or_intrin
  "vpx_dsp"
  "vpx_scale"
  "vpx_ports"
  "vp8_common"
  "vp8_encoder"
  "vp8_decoder"
  "vp9_common"
  "vp9_encoder"
  "vp9_decoder"
)

foreach(_vpx_target IN LISTS _vpx_c_targets _vpx_targets_with_asm_or_intrin) 
  if (TARGET ${_vpx_target})
    target_include_directories(${_vpx_target} PUBLIC
        $<BUILD_INTERFACE:${VPX_ROOT}>
        $<BUILD_INTERFACE:${VPX_CONFIG_DIR}>
        $<BUILD_INTERFACE:${VPX_ROOT}/${_vpx_target}>
      )
    target_sources(libvpx PRIVATE $<TARGET_OBJECTS:${_vpx_target}>)
  endif ()
endforeach()

foreach(_vpx_target IN LISTS _vpx_targets_with_asm_or_intrin) 
  foreach(asm_type in "sse" "sse2" "ssse3" "sse4_1" "avx" "avx2" "avx512" "mmx" "neon")
    if (TARGET ${_vpx_target}_${asm_type}_intrinsics)
      target_sources(libvpx PRIVATE $<TARGET_OBJECTS:${_vpx_target}_${asm_type}_intrinsics>)
    endif ()  
    if (TARGET ${_vpx_target}_${asm_type}_asm)
      target_sources(libvpx PRIVATE $<TARGET_OBJECTS:${_vpx_target}_${asm_type}_asm>)
    endif ()
  endforeach()
endforeach()

if (HAVE_PTHREAD_H AND CONFIG_MULTITHREAD)
  find_package(Threads)
  target_link_libraries(libvpx PRIVATE Threads::Threads)
endif ()
if (NOT MSVC)
  target_link_libraries(libvpx PRIVATE m)
endif ()

# let's export a few select symbols on all platforms.
if(BUILD_SHARED_LIBS)
  if(NOT WIN32 AND NOT APPLE)
    # The -z defs linker option reports unresolved symbol references from object
    # files when building a shared library.
    target_link_options(libvpx PRIVATE LINKER:-z,defs)
  endif()
  include("${VPX_ROOT}/build/cmake/vpx_export_symbols.cmake")
  setup_exports_target(libvpx "${LIBVPX_MANGLE_PREFIX}")
endif()

vtk_module_install_headers(
  FILES ${vpx_installable_headers}
  SUBDIR "vtkstreaminglibvpx"
)

vtk_module_include(VTKStreaming::libvpx
  PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
)
