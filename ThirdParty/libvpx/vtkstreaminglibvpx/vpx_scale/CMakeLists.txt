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

add_rtcd_build_step(
  "${VPX_ROOT}/vpx_scale/vpx_scale_rtcd.pl"
  "${VPX_CONFIG_DIR}/vpx_scale_rtcd.h"
  "${VPX_ROOT}/vpx_scale/vpx_scale_rtcd.c"
  "vpx_scale_rtcd"
)

list(APPEND vpx_scale_c_sources "${VPX_ROOT}/vpx_scale/vpx_scale_rtcd.c")

add_library(vpx_scale OBJECT
  ${vpx_scale_headers}
  ${vpx_scale_c_sources}
)
