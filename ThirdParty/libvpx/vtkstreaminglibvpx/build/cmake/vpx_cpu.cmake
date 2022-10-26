# Detect target CPU.
if(NOT VPX_TARGET_CPU)
  string(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" cpu_lowercase)
  if(cpu_lowercase STREQUAL "amd64" OR cpu_lowercase STREQUAL "x86_64")
    if(CMAKE_SIZEOF_VOID_P EQUAL 4)
      set(VPX_TARGET_CPU "x86")
    elseif(CMAKE_SIZEOF_VOID_P EQUAL 8)
      set(VPX_TARGET_CPU "x86_64")
    else()
      message(
        FATAL_ERROR "--- Unexpected pointer size (${CMAKE_SIZEOF_VOID_P}) for\n"
                    "      CMAKE_SYSTEM_NAME=${CMAKE_SYSTEM_NAME}\n"
                    "      CMAKE_SYSTEM_PROCESSOR=${CMAKE_SYSTEM_PROCESSOR}\n"
                    "      CMAKE_GENERATOR=${CMAKE_GENERATOR}\n")
    endif()
  elseif(cpu_lowercase STREQUAL "i386" OR cpu_lowercase STREQUAL "x86")
    set(VPX_TARGET_CPU "x86")
  elseif(cpu_lowercase MATCHES "^arm")
    set(VPX_TARGET_CPU "${cpu_lowercase}")
  elseif(cpu_lowercase MATCHES "aarch64")
    set(VPX_TARGET_CPU "arm64")
  elseif(cpu_lowercase MATCHES "^ppc")
    set(VPX_TARGET_CPU "ppc")
  else()
    message(WARNING "The architecture ${CMAKE_SYSTEM_PROCESSOR} is not "
                    "supported, falling back to the generic target")
    set(VPX_TARGET_CPU "generic")
  endif()
endif()
