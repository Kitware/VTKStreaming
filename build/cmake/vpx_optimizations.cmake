if(VPX_BUILD_CMAKE_VPX_OPTIMIZATION_CMAKE_)
  return()
endif() # VPX_BUILD_CMAKE_VPX_OPTIMIZATION_CMAKE_
set(VPX_BUILD_CMAKE_VPX_OPTIMIZATION_CMAKE_ 1)

# Translate $flag to one which MSVC understands, and write the new flag to the
# variable named by $translated_flag (or unset it, when MSVC needs no flag).
function(get_msvc_intrinsic_flag flag translated_flag)
  if("${flag}" STREQUAL "-mavx")
    set(${translated_flag} "/arch:AVX" PARENT_SCOPE)
  elseif("${flag}" STREQUAL "-mavx2")
    set(${translated_flag} "/arch:AVX2" PARENT_SCOPE)
  else()

    # MSVC does not need flags for intrinsics flavors other than AVX/AVX2.
    unset(${translated_flag} PARENT_SCOPE)
  endif()
endfunction()

# Adds a new intrinsic library target of type OBJECT, named ${base_target_name}_${intrinsic_name}_intrinsics
function(create_intrinsics_object_target intrinsic_flag intrinsic_name base_target_name)
  set(intrin_name ${base_target_name}_${intrinsic_name})
  set(intrin_target ${intrin_name}_intrinsics)
  add_library(${intrin_target} OBJECT
    ${${intrin_name}_headers}
    ${${intrin_name}_c_sources}
    $<TARGET_OBJECTS:${base_target_name}>
  )
  target_include_directories(${intrin_target} PRIVATE
    "${VPX_ROOT}"
    "${VPX_ROOT}/${base_target_name}"
    "${VPX_CONFIG_DIR}"
  )
  set_property(TARGET ${intrin_target} PROPERTY FOLDER ${VPX_TARGET_CPU})
  if (MSVC)
    get_msvc_intrinsic_flag("${intrinsic_flag}" "intrinsic_flag")
  endif ()
  if ("${intrinsic_flag}" STREQUAL "-mavx2")
  endif ()

  if (intrinsic_flag)
    separate_arguments(intrinsic_flag)
    target_compile_options(${intrin_target} PUBLIC ${intrinsic_flag})
  endif ()
endfunction()

# Adds a new assembly library target of type OBJECT, named ${base_target_name}_${asm_name}_asm
function(create_assembly_object_target asm_type base_target_name)
  set(asm_name ${base_target_name}_${asm_type})
  set(asm_target ${asm_name}_asm)
  add_library(${asm_target} OBJECT
    ${${asm_name}_asm_sources}
  )
  set_property(TARGET ${intrin_target} PROPERTY FOLDER ${VPX_TARGET_CPU})
  target_include_directories(${asm_target} PRIVATE
    "${VPX_ROOT}"
    "${VPX_ROOT}/${base_target_name}"
    "${VPX_CONFIG_DIR}"
  )
endfunction()
