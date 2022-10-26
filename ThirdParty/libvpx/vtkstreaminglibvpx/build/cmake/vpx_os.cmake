set(VPX_TARGET_SYSTEM ${CMAKE_SYSTEM_NAME})
if(VPX_TARGET_SYSTEM MATCHES "Darwin\|Linux\|Windows")
    set(CONFIG_OS_SUPPORT 1)
endif()

include ("${VPX_ROOT}/build/cmake/vpx_compiler_tests.cmake")
# Test compiler support.
vpx_get_inline("INLINE")

include(FindThreads)

if (BUILD_SHARED_LIBS)
  set(CONFIG_SHARED 1)
endif ()
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CONFIG_PIC 1)

# Don't just check for pthread.h, but use the result of the full pthreads
# including a linking check in FindThreads above.
set(HAVE_PTHREAD_H ${CMAKE_USE_PTHREADS_INIT})
vpx_check_c_compiles("unistd_check" "#include <unistd.h>" HAVE_UNISTD_H)

# if (VPX_TARGET_SYSTEM MATCHES "Windows")
#   set (CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS OFF)
# endif ()
