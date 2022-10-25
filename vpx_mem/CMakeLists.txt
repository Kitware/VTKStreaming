# Commented out lines that begin with `MEM_SRCS-`
# correspond to features that we don't cater to.
# If upstream already commented them, we use a double `##`

set(vpx_mem_headers "${CMAKE_CURRENT_LIST_DIR}/vpx_mem.h")
set(vpx_mem_internal_headers "${CMAKE_CURRENT_LIST_DIR}/include/vpx_mem_intrnl.h")
set(vpx_mem_c_sources "${CMAKE_CURRENT_LIST_DIR}/vpx_mem.c")

add_library(vpx_mem OBJECT
  ${vpx_mem_headers}
  ${vpx_mem_internal_headers}
  ${vpx_mem_c_sources}
)
