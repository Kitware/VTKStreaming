# Commented out lines that begin with `SCALE_SRCS-`
# correspond to architecture/asm extension that we don't cater to.
# If upstream already commented them, we use a double `##`

set(vpx_scale_headers)
set(vpx_scale_c_sources)

list(APPEND vpx_scale_headers "${CMAKE_CURRENT_LIST_DIR}/yv12config.h")

if (CONFIG_SPATIAL_RESAMPLING)
  list(APPEND vpx_scale_headers "${CMAKE_CURRENT_LIST_DIR}/vpx_scale.h")
  list(APPEND vpx_scale_c_sources "${CMAKE_CURRENT_LIST_DIR}/generic/vpx_scale.c")
endif ()

list(APPEND vpx_scale_c_sources
  "${CMAKE_CURRENT_LIST_DIR}/generic/yv12config.c"
  "${CMAKE_CURRENT_LIST_DIR}/generic/yv12extend.c"
)

if (CONFIG_SPATIAL_RESAMPLING)
  list(APPEND vpx_scale_c_sources "${CMAKE_CURRENT_LIST_DIR}/generic/gen_scalers.c")
endif ()

#mips(dspr2)
# SCALE_SRCS-$(HAVE_DSPR2)  += mips/dspr2/yv12extend_dspr2.c

if (VPX_ARCH_ARM)
  file(COPY "${VPX_ROOT}/build/rtcd_artifacts/rtcd_neon/vpx_scale_rtcd.h" DESTINATION "${VPX_CONFIG_DIR}")
elseif (VPX_ARCH_X86 OR VPX_ARCH_X86_64)
  file(COPY "${VPX_ROOT}/build/rtcd_artifacts/rtcd_x86/vpx_scale_rtcd.h" DESTINATION "${VPX_CONFIG_DIR}")
endif ()
list(APPEND vpx_scale_headers "${VPX_CONFIG_DIR}/vpx_scale_rtcd.h")
list(APPEND vpx_scale_c_sources "${VPX_ROOT}/vpx_scale/vpx_scale_rtcd.c")

add_library(vpx_scale OBJECT
  ${vpx_scale_headers}
  ${vpx_scale_c_sources}
)
