# Commented out lines that begin with `PORTS_SRCS-`
# correspond to architecture/asm extension that we don't cater to.
# If upstream already commented them, we use a double `##`

set(vpx_util_headers
  "${CMAKE_CURRENT_LIST_DIR}/vpx_atomics.h"
  "${CMAKE_CURRENT_LIST_DIR}/vpx_thread.h"
  "${CMAKE_CURRENT_LIST_DIR}/endian_inl.h"
  "${CMAKE_CURRENT_LIST_DIR}/vpx_write_yuv_frame.h"
  "${CMAKE_CURRENT_LIST_DIR}/vpx_timestamp.h"
)
set(vpx_util_c_sources
  "${CMAKE_CURRENT_LIST_DIR}/vpx_thread.c"
  "${CMAKE_CURRENT_LIST_DIR}/vpx_write_yuv_frame.c"
)

if (CONFIG_BITSTREAM_DEBUG OR CONFIG_MISMATCH_DEBUG)
  list(APPEND vpx_util_headers "${CMAKE_CURRENT_LIST_DIR}/vpx_debug_util.h")
  list(APPEND vpx_util_c_sources "${CMAKE_CURRENT_LIST_DIR}/vpx_debug_util.c")
endif ()

add_library(vpx_util OBJECT
  ${vpx_util_headers}
  ${vpx_util_c_sources}
)
