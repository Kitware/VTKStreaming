# Commented out lines that begin with `PORTS_SRCS-`
# correspond to architecture/asm extension that we don't cater to.
# If upstream already commented them, we use a double `##`

set(vpx_ports_headers)
set(vpx_ports_c_sources)
set(vpx_ports_asm_sources)

set(vpx_ports_mmx_headers)
set(vpx_ports_mmx_c_sources)
set(vpx_ports_mmx_asm_sources)

list(APPEND vpx_ports_headers
  "${CMAKE_CURRENT_LIST_DIR}/bitops.h"
  "${CMAKE_CURRENT_LIST_DIR}/compiler_attributes.h"
  "${CMAKE_CURRENT_LIST_DIR}/mem.h"
  "${CMAKE_CURRENT_LIST_DIR}/msvc.h"
  "${CMAKE_CURRENT_LIST_DIR}/static_assert.h"
  "${CMAKE_CURRENT_LIST_DIR}/system_state.h"
  "${CMAKE_CURRENT_LIST_DIR}/vpx_timer.h"
)
if (VPX_ARCH_X86)
  list(APPEND vpx_ports_mmx_c_sources "${CMAKE_CURRENT_LIST_DIR}/emms_mmx.c")
endif ()
if (VPX_ARCH_X86_64)
  # Visual Studio x64 does not support the _mm_empty() intrinsic.
  list(APPEND vpx_ports_mmx_asm_sources "${CMAKE_CURRENT_LIST_DIR}/emms_mmx.asm")
endif ()

if (VPX_ARCH_X86_64)
  if (MSVC)
   list(APPEND vpx_ports_asm_sources "${CMAKE_CURRENT_LIST_DIR}/float_control_word.asm")
  endif ()
endif ()

if (VPX_ARCH_X86 OR VPX_ARCH_X86_64)
  # when there are no c files and ASM_NASM was enabled instead of project(..)
  # cmake is unable to determine linker target language.
  file(TOUCH "${VPX_CONFIG_DIR}/placeholder_vpx_ports.c")
  list(APPEND vpx_ports_headers "${CMAKE_CURRENT_LIST_DIR}/x86.h")
  list(APPEND vpx_ports_c_sources "${VPX_CONFIG_DIR}/placeholder_vpx_ports.c")
  list(APPEND vpx_ports_asm_sources 
    "${CMAKE_CURRENT_LIST_DIR}/x86_abi_support.asm"
    "${VPX_ROOT}/third_party/x86inc/x86inc.asm"
  )
endif ()

if (VPX_ARCH_ARM)
  list(APPEND vpx_ports_headers "${CMAKE_CURRENT_LIST_DIR}/arm.h")
  list(APPEND vpx_ports_c_sources "${CMAKE_CURRENT_LIST_DIR}/arm_cpudetect.c")
endif ()

# PORTS_SRCS-$(VPX_ARCH_PPC) += ppc_cpudetect.c
# PORTS_SRCS-$(VPX_ARCH_PPC) += ppc.h

# PORTS_SRCS-$(VPX_ARCH_MIPS) += mips_cpudetect.c
# PORTS_SRCS-$(VPX_ARCH_MIPS) += mips.h

# PORTS_SRCS-$(VPX_ARCH_LOONGARCH) += loongarch_cpudetect.c
# PORTS_SRCS-$(VPX_ARCH_LOONGARCH) += loongarch.h

# ifeq ($(VPX_ARCH_MIPS), yes)
# PORTS_SRCS-yes += asmdefs_mmi.h
# endif

add_library(vpx_ports OBJECT
  ${vpx_ports_headers}
  ${vpx_ports_c_sources}
  ${vpx_ports_asm_sources}
)
if (HAVE_MMX AND vpx_ports_mmx_asm_sources)
  create_assembly_object_target("mmx" "vpx_ports")    
endif ()
