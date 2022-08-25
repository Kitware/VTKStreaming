#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="libwebm"
readonly ownership="Libwebm Upstream <kwrobot@kitware.com>"
readonly subtree="ThirdParty/$name/vtk$name"
readonly repo="https://gitlab.kitware.com/third-party/libwebm.git"
readonly tag="for/vtk-20220825-1.0.0.28"
readonly paths="
build
common/*.cc
common/*.h
mkvmuxer
mkvparser

CMakeLists.vtk.txt

CONTRIBUTING.md
AUTHORS.TXT
LICENSE.TXT
PATENTS.TXT

README.kitware.md
README.libwebm
.gitattributes
"

extract_source () {
    git_archive
    pushd "$extractdir/$name-reduced"
    mv -v CMakeLists.vtk.txt CMakeLists.txt
    popd
}

. "${BASH_SOURCE%/*}/../update-common.sh"
