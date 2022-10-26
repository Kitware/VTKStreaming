# Used to determine the version for libvpx source using "git describe", if git
# is found and CHANGELOG. On success sets following variables in caller's scope:
# ${var_prefix}_VERSION       = "v<major>.<minor>.<patch>-<extra>"
# ${var_prefix}_VERSION_MAJOR = <major>
# ${var_prefix}_VERSION_MINOR = <minor>
# ${var_prefix}_VERSION_PATCH = <patch>
# ${var_prefix}_VERSION_EXTRA = <extra>

# Arguments are:
#   source_dir: Source directory
#   git_command: git executable
#   var_prefix: prefix for variables e.g. "VPX"
function(determine_version source_dir git_command var_prefix)
  unset (tmp_VERSION)
  set(output "")
  if (EXISTS ${git_command} AND EXISTS "${source_dir}/.git")
    execute_process(
      COMMAND ${git_command} describe --match "v[0-9]*"
      WORKING_DIRECTORY ${source_dir}
      RESULT_VARIABLE result
      OUTPUT_VARIABLE output
      OUTPUT_STRIP_TRAILING_WHITESPACE
      ERROR_STRIP_TRAILING_WHITESPACE
    )
    if (NOT result EQUAL "0")
      # git describe failed (bad return code).
      set(output "")
    endif ()
  else ()
    # source archive wil have CHANGELOG. let's read version off the first line.
    if (EXISTS "${source_dir}/CHANGELOG")
      set(_changelog_lines)
      set(_line)
      file(STRINGS "${source_dir}/CHANGELOG" _changelog_lines)
      list(GET _changelog_lines 0 _line)
      string(REPLACE " " ";" words ${_line})
      list(GET words 1 output)
    endif ()
  endif ()
  extract_version_components("${output}" tmp)
  foreach(suffix VERSION VERSION_MAJOR VERSION_MINOR VERSION_PATCH
                  VERSION_EXTRA)
    set(${var_prefix}_${suffix} ${tmp_${suffix}} PARENT_SCOPE)
  endforeach()
endfunction()

# Extracts components from a version string. See determine_version() for usage.
function(extract_version_components version_string var_prefix)
  string(REGEX MATCH "^v?(([0-9]+)\\.([0-9]+)\\.([0-9]+)-?(.*))$"
    version_matches "${version_string}")
  if(CMAKE_MATCH_0)
    # note, we don't use CMAKE_MATCH_0 for `full` since it may or may not have
    # the `v` prefix.
    set(full ${CMAKE_MATCH_1})
    set(major ${CMAKE_MATCH_2})
    set(minor ${CMAKE_MATCH_3})
    set(patch ${CMAKE_MATCH_4})
    set(extra ${CMAKE_MATCH_5})

    if (extra)
      set(${var_prefix}_VERSION "v${major}.${minor}.${patch}-${extra}" PARENT_SCOPE)
    else ()
      set(${var_prefix}_VERSION "v${major}.${minor}.${patch}" PARENT_SCOPE)
    endif ()

    set(${var_prefix}_VERSION_MAJOR ${major} PARENT_SCOPE)
    set(${var_prefix}_VERSION_MINOR ${minor} PARENT_SCOPE)
    set(${var_prefix}_VERSION_PATCH ${patch} PARENT_SCOPE)
    set(${var_prefix}_VERSION_EXTRA ${extra} PARENT_SCOPE)
  endif ()
endfunction()
