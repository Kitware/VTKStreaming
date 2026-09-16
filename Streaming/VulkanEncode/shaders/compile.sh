#!/bin/sh
# SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
# SPDX-License-Identifier: Apache-2.0
#
# Regenerates the checked-in SPIR-V headers from the GLSL compute shaders in this directory.
# Needs `glslang` (or the older `glslangValidator`) from the Vulkan SDK / KhronosGroup/glslang
# releases; set GLSLANG to point at it if it is not on PATH.
set -eu
cd "$(dirname "$0")"
GLSLANG="${GLSLANG:-$(command -v glslang || command -v glslangValidator)}"
for src in *.comp; do
  name="${src%.comp}"
  out="../${name}.h"
  tmp="$(mktemp)"
  "$GLSLANG" -V -g0 --target-env vulkan1.3 --vn "${name}" -o "${tmp}" "${src}" > /dev/null
  {
    echo "// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc."
    echo "// SPDX-License-Identifier: Apache-2.0"
    echo "// Generated from shaders/${src} by shaders/compile.sh; do not edit."
    echo "#ifndef ${name}_h"
    echo "#define ${name}_h"
    echo "#include <cstdint>"
    grep -v '^[[:space:]]*//\|#pragma once' "${tmp}"
    echo "#endif // ${name}_h"
    echo "// VTK-HeaderTest-Exclude: ${name}.h"
  } > "${out}"
  rm -f "${tmp}"
done
