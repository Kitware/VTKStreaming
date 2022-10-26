# Commented out lines that begin with `API_SRCS-`
# correspond to features that we don't cater to.
# If upstream already commented them, we use a double `##`

set(vpx_codec_headers PARENT_SCOPE)
set(vpx_codec_c_sources PARENT_SCOPE)
set(vpx_codec_internal_headers PARENT_SCOPE)

if (CONFIG_VP8_ENCODER OR CONFIG_VP9_ENCODER)
  list(APPEND vpx_codec_headers 
    "${CMAKE_CURRENT_LIST_DIR}/vp8.h"
    "${CMAKE_CURRENT_LIST_DIR}/vp8cx.h"
  )
  # API_DOC_SRCS-$(CONFIG_VP8_DECODER) += vp8.h
  # API_DOC_SRCS-$(CONFIG_VP8_DECODER) += vp8dx.h
endif ()

if (CONFIG_VP8_DECODER OR CONFIG_VP9_DECODER)
  list(APPEND vpx_codec_headers
    "${CMAKE_CURRENT_LIST_DIR}/vp8.h"
    "${CMAKE_CURRENT_LIST_DIR}/vp8dx.h"
  )
  # API_DOC_SRCS-$(CONFIG_VP8_DECODER) += vp8.h
  # API_DOC_SRCS-$(CONFIG_VP8_DECODER) += vp8dx.h
endif ()

# API_DOC_SRCS-yes += vpx_codec.h
# API_DOC_SRCS-yes += vpx_decoder.h
# API_DOC_SRCS-yes += vpx_encoder.h
# API_DOC_SRCS-yes += vpx_ext_ratectrl.h
# API_DOC_SRCS-yes += vpx_frame_buffer.h
# API_DOC_SRCS-yes += vpx_image.h

list(APPEND vpx_codec_headers
  "${CMAKE_CURRENT_LIST_DIR}/vpx_codec.h"
  "${CMAKE_CURRENT_LIST_DIR}/vpx_decoder.h"
  "${CMAKE_CURRENT_LIST_DIR}/vpx_encoder.h"
  "${CMAKE_CURRENT_LIST_DIR}/vpx_ext_ratectrl.h"
  "${CMAKE_CURRENT_LIST_DIR}/vpx_frame_buffer.h"
  "${CMAKE_CURRENT_LIST_DIR}/vpx_image.h"
  "${CMAKE_CURRENT_LIST_DIR}/vpx_integer.h"
  "${VPX_CONFIG_DIR}/vpx_config.h"
  "${VPX_CONFIG_DIR}/vpx_mangle.h"
  "${VPX_CONFIG_DIR}/vpx_version.h"
)
list(APPEND vpx_codec_internal_headers
  "${CMAKE_CURRENT_LIST_DIR}/internal/vpx_codec_internal.h"
  "${CMAKE_CURRENT_LIST_DIR}/internal/vpx_ratectrl_rtc.h"
)
list(APPEND vpx_codec_c_sources
  "${CMAKE_CURRENT_LIST_DIR}/src/vpx_codec.c"
  "${CMAKE_CURRENT_LIST_DIR}/src/vpx_decoder.c"
  "${CMAKE_CURRENT_LIST_DIR}/src/vpx_encoder.c"
  "${CMAKE_CURRENT_LIST_DIR}/src/vpx_image.c"
  "${VPX_CONFIG_DIR}/vpx_codec_config.c"
)

add_library(vpx_codec OBJECT  
  ${vpx_codec_headers}
  ${vpx_codec_internal_headers}
  ${vpx_codec_c_sources}
)

set(vpx_installable_headers ${vpx_codec_headers} PARENT_SCOPE)
