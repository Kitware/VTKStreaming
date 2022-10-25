if (VPX_BUILD_CMAKE_EXPORTS_CMAKE_)
  return()
endif() # VPX_BUILD_CMAKE_EXPORTS_CMAKE_
set (VPX_BUILD_CMAKE_EXPORTS_CMAKE_ 1)

include("${VPX_ROOT}/build/cmake/vpx_os.cmake")

list(APPEND VPX_EXPORTS_SOURCES "${VPX_ROOT}/vpx/exports_com")
if (CONFIG_VP8_DECODER)
  list(APPEND VPX_EXPORTS_SOURCES
    "${VPX_ROOT}/vp8/exports_dec"
  )
endif ()
if (CONFIG_VP8_ENCODER)
  list(APPEND VPX_EXPORTS_SOURCES
    "${VPX_ROOT}/vp8/exports_enc"
  )
endif ()
if (CONFIG_VP9_DECODER)
  list(APPEND VPX_EXPORTS_SOURCES
    "${VPX_ROOT}/vp9/exports_dec"
  )
endif ()
if (CONFIG_VP9_ENCODER)
  list(APPEND VPX_EXPORTS_SOURCES
    "${VPX_ROOT}/vp9/exports_enc"
  )
endif ()
if (CONFIG_DECODERS)
  list(APPEND VPX_EXPORTS_SOURCES
    "${VPX_ROOT}/vpx/exports_dec")
endif ()
if (CONFIG_ENCODERS)
  list(APPEND VPX_EXPORTS_SOURCES
    "${VPX_ROOT}/vpx/exports_enc")
endif ()

# Creates the custom target which handles generation of the symbol export lists.
function(setup_exports_target base_target mangle_prefix)
  if(APPLE)
    set(symbol_file_ext "syms")
  elseif(WIN32)
    set(symbol_file_ext "def")
  else()
    set(symbol_file_ext "ver")
  endif()

  set(VPX_SYM_FILE "${VPX_CONFIG_DIR}/libvpx.${symbol_file_ext}")
  file(REMOVE "${VPX_SYM_FILE}")

  if("${VPX_TARGET_SYSTEM}" STREQUAL "Darwin")
    set(symbol_prefix "_")
  elseif("${VPX_TARGET_SYSTEM}" STREQUAL "Windows")
    file(WRITE "${VPX_SYM_FILE}" "LIBRARY ${CMAKE_SHARED_LIBRARY_PREFIX}vpx\n"
                                "EXPORTS\n")
  else()
    file(WRITE "${VPX_SYM_FILE}" "{\nglobal:\n")
    set(symbol_suffix ";")
  endif()

  foreach(export_file ${VPX_EXPORTS_SOURCES})
    file(STRINGS "${export_file}" exported_file_data)
    set(exported_symbols "${exported_symbols} ${exported_file_data};")
    string(STRIP "${exported_symbols}" exported_symbols)
  endforeach()

  foreach(exported_symbol ${exported_symbols})
    string(STRIP "${exported_symbol}" exported_symbol)
    if("${VPX_TARGET_SYSTEM}" STREQUAL "Windows")
      string(SUBSTRING ${exported_symbol} 0 4 export_type)
      string(COMPARE EQUAL "${export_type}" "data" is_data)
      if(is_data)
        set(symbol_suffix " DATA")
      else()
        set(symbol_suffix "")
      endif()
    endif()
    string(REGEX REPLACE "text \|data " "" "exported_symbol" "${exported_symbol}")
    set(exported_symbol "  ${symbol_prefix}${mangle_prefix}${exported_symbol}${symbol_suffix}")
    file(APPEND "${VPX_SYM_FILE}" "${exported_symbol}\n")
  endforeach()

  if("${VPX_SYM_FILE}" MATCHES "ver$")
    file(APPEND "${VPX_SYM_FILE}" " \nlocal:\n  *;\n};")
  endif()
  target_sources(${base_target} PRIVATE "${VPX_SYM_FILE}")
  if(APPLE)
    set_property(TARGET ${base_target}
                 APPEND_STRING
                 PROPERTY LINK_FLAGS "-exported_symbols_list ${VPX_SYM_FILE}")
  elseif(MSVC)
    set_property(TARGET ${base_target}
                  APPEND_STRING
                  PROPERTY LINK_FLAGS "/def:${VPX_SYM_FILE}")
  else()
    set_property(TARGET ${base_target}
                 APPEND_STRING
                 PROPERTY LINK_FLAGS "-Wl,--version-script,${VPX_SYM_FILE}")
  endif()
endfunction()
