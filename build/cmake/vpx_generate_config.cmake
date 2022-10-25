# configure `vpx_config.[h, asm]` with options from `vpx_options.cmake`
configure_file(${VPX_ROOT}/vpx_config.h.in ${VPX_CONFIG_DIR}/vpx_config.h)
file(STRINGS ${VPX_CONFIG_DIR}/vpx_config.h _vpx_config_lines)

foreach (_line ${_vpx_config_lines})
  string(FIND ${_line} "#define" _has_define)
  if (${_has_define} EQUAL -1)
    continue()
  endif ()
  string(REPLACE " " ";" words ${_line})
  list(POP_BACK words flag)
  if (flag EQUAL 0 OR flag EQUAL 1)
    list(GET words 1 config_attribute)
    set(asm_line "${config_attribute} equ ${flag}\n")
    list(APPEND _vpx_config_asm_content ${asm_line})
  endif()
endforeach ()

file(WRITE ${VPX_CONFIG_DIR}/vpx_config.asm ${_vpx_config_asm_content})
