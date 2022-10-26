if(VPX_BUILD_CMAKE_COMPILER_TESTS_CMAKE_)
  return()
endif() # VPX_BUILD_CMAKE_COMPILER_TESTS_CMAKE_
set(VPX_BUILD_CMAKE_COMPILER_TESTS_CMAKE_ 1)

include(CheckCSourceCompiles)
# The basic main() function used in all compile tests.
set(VPX_C_MAIN "\nint main(void) { return 0; }")
# Strings containing the names of passed and failed tests.
set(VPX_C_PASSED_TESTS)
set(VPX_C_FAILED_TESTS)

function(vpx_push_var var new_value)
  set(SAVED_${var} ${${var}} PARENT_SCOPE)
  set(${var} "${${var}} ${new_value}" PARENT_SCOPE)
endfunction()

function(vpx_pop_var var)
  set(var ${SAVED_${var}} PARENT_SCOPE)
  unset(SAVED_${var} PARENT_SCOPE)
endfunction()

# Confirms $test_source compiles and stores $test_name in one of
# $VPX_C_PASSED_TESTS or $VPX_C_FAILED_TESTS depending on out come. When the
# test passes $result_var is set to 1. When it fails $result_var is unset. The
# test is not run if the test name is found in either of the passed or failed
# test variables.
function(vpx_check_c_compiles test_name test_source result_var)
  if(DEBUG_CMAKE_DISABLE_COMPILER_TESTS)
    return()
  endif()

  unset(C_TEST_PASSED CACHE)
  unset(C_TEST_FAILED CACHE)
  string(FIND "${VPX_C_PASSED_TESTS}" "${test_name}" C_TEST_PASSED)
  string(FIND "${VPX_C_FAILED_TESTS}" "${test_name}" C_TEST_FAILED)
  if(${C_TEST_PASSED} EQUAL -1 AND ${C_TEST_FAILED} EQUAL -1)
    unset(C_TEST_COMPILED CACHE)
    message("Running C compiler test: ${test_name}")
    check_c_source_compiles("${test_source} ${VPX_C_MAIN}" C_TEST_COMPILED)
    set(${result_var} ${C_TEST_COMPILED} PARENT_SCOPE)

    if(C_TEST_COMPILED)
      set(VPX_C_PASSED_TESTS
          "${VPX_C_PASSED_TESTS} ${test_name}"
          CACHE STRING "" FORCE)
    else()
      set(VPX_C_FAILED_TESTS
          "${VPX_C_FAILED_TESTS} ${test_name}"
          CACHE STRING "" FORCE)
      message("C Compiler test ${test_name} failed.")
    endif()
  elseif(NOT ${C_TEST_PASSED} EQUAL -1)
    set(${result_var} 1 PARENT_SCOPE)
  else() # ${C_TEST_FAILED} NOT EQUAL -1
    unset(${result_var} PARENT_SCOPE)
  endif()
endfunction()

# When inline support is detected for the current compiler the supported
# inlining keyword is written to $result in caller scope.
function(vpx_get_inline result)
  vpx_check_c_compiles("inline_check_1"
                            "static inline void function(void) {}"
                            HAVE_INLINE_1)
  if(HAVE_INLINE_1 EQUAL 1)
    set(${result} "inline" PARENT_SCOPE)
    return()
  endif()

  # Check __inline.
  vpx_check_c_compiles("inline_check_2"
                            "static __inline void function(void) {}"
                            HAVE_INLINE_2)
  if(HAVE_INLINE_2 EQUAL 1)
    set(${result} "__inline" PARENT_SCOPE)
  endif()
endfunction()
