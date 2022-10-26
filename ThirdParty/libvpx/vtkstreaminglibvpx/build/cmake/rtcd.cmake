# Adds build command for generation of rtcd C source files using
# build/cmake/rtcd.pl. $config is the input perl file, $output is the output C
# include file, $source is the C source file, and $symbol is used for the symbol
# argument passed to rtcd.pl.
# TODO: Verify

if("${VPX_TARGET_CPU}" MATCHES "^arm")
  set(VPX_ARCH_ARM 1)

  if(ENABLE_NEON)
    set(HAVE_NEON 1)
  else()
    set(HAVE_NEON 0)
    set(VPX_RTCD_FLAGS ${VPX_RTCD_FLAGS} --disable-neon)
  endif()
elseif("${VPX_TARGET_CPU}" MATCHES "^x86")
  if("${VPX_TARGET_CPU}" STREQUAL "x86")
    set(VPX_ARCH_X86 1)
  elseif("${VPX_TARGET_CPU}" STREQUAL "x86_64")
    set(VPX_ARCH_X86_64 1)
  endif()

  set(X86_FLAVORS "MMX;SSE;SSE2;SSE3;SSSE3;SSE4_1;AVX;AVX2;AVX512")
  foreach(flavor ${X86_FLAVORS})
    if(ENABLE_${flavor} AND NOT disable_remaining_flavors)
      set(HAVE_${flavor} 1)
    else()
      set(disable_remaining_flavors 1)
      set(HAVE_${flavor} 0)
      string(TOLOWER ${flavor} flavor)
      set(VPX_RTCD_FLAGS ${VPX_RTCD_FLAGS} --disable-${flavor})
    endif()
  endforeach()
endif()

find_package(Perl REQUIRED)

function(add_rtcd_build_step config output source symbol)
  add_custom_command(
    OUTPUT ${output}
    COMMAND ${PERL_EXECUTABLE} ARGS "${VPX_ROOT}/build/cmake/rtcd.pl"
            --arch=${VPX_TARGET_CPU}
            --sym=${symbol} ${VPX_RTCD_FLAGS}
            --config=${VPX_CONFIG_DIR}/vpx_config.h ${config} > ${output}
    DEPENDS ${config} ${VPX_CONFIG_DIR}/vpx_config.h
    COMMENT "Generating ${output}"
    WORKING_DIRECTORY ${VPX_CONFIG_DIR}
    VERBATIM)
  set_property(SOURCE ${source} PROPERTY OBJECT_DEPENDS ${output})
  set_property(SOURCE ${output} PROPERTY GENERATED TRUE)
endfunction()
