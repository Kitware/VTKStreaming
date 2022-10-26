#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="libvpx"
readonly ownership="Libvpx Upstream <kwrobot@kitware.com>"
readonly subtree="ThirdParty/$name/vtkstreaming$name"
readonly repo="https://gitlab.kitware.com/third-party/libvpx.git"
readonly tag="for/vtk-20221025.5-v1.12.0"
readonly paths="
build/cmake
third_party/x86inc/x86inc.asm
vp8
vp9
vpx
vpx_dsp
vpx_mem
vpx_scale
vpx_ports
vpx_util

vpx_config.h.in
vpx_codec_config.c.in
vpx_mangle.h.in
vpx_version.h.in

CMakeLists.vtk.txt

CONTRIBUTING.md
AUTHORS
LICENSE
PATENTS
CHANGELOG

README.kitware.md
README
.gitattributes
"

extract_source () {
    git_archive
    pushd "$extractdir/$name-reduced"
    mv -v CMakeLists.vtk.txt CMakeLists.txt
    # for vtkstreaming
    sed -i 's/LIBVPX_MANGLE_PREFIX \"vtk_\"/LIBVPX_MANGLE_PREFIX \"vtkstreaming_\"/g' CMakeLists.txt
    sed -i 's/VTK::libvpx/VTKStreaming::libvpx/g' CMakeLists.txt
    popd
}

. "${BASH_SOURCE%/*}/../update-common.sh"
