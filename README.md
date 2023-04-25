# VTKStreaming

This module provides classes to encode and stream frames
from a VTK OpenGL render window using video codecs. 
It supports video encoding with VP9 (through [libvpx](https://chromium.googlesource.com/webm/libvpx/))
and H.264/H.265 (through [NVENC](https://developer.nvidia.com/nvidia-video-codec-sdk/download)). The VP9 bitstream can be saved to `.webm` files playable by most media players. This repository does not provide classes to multiplex H.264/H.265 bitstreams into MP4 videos due to license incompatibilities.

# Requirements

1. Clone and build VTK 9.2+ from source.
2. If you plan on developing/running tests, `git-lfs` is needed to download the test images, videos. Simply execute `git lfs install` in your terminal and you're all set.
3. If your system has a supported nvidia gpu, [libvpx](https://chromium.googlesource.com/webm/libvpx/) is not required and you may skip this step by setting the CMake flag `VTKSTREAMING_USE_LIBVPX=OFF`. If you're running on macOS or a system without an NVIDIA GPU, you'll need `nasm` in order to compile `x86_64`/`arm64` assembly sources from libvpx library. You may download `nasm` binaries for linux, windows and mac from [nasm.us](https://nasm.us) and append the `PATH` variable with the location of `nasm` on your filesystem. If that link is down, try https://www.nasm.us/pub/nasm/stable/. For users of RPM-based Linux distributions (e.g. Fedora, Red Hat, SUSE, ...), you can download the official NASM builds using `dnf` or `yum` by installing [nasm.repo](https://www.nasm.us/nasm.repo) in your `/etc/yum/yum.repos.d` directory.

# Instructions
1. Clone the repository, setup git hooks if you intend to contribute.
    ```sh
    git clone https://gitlab.kitware.com/async/vtkstreaming.git
    ./Utilities/SetupForDevelopment.sh
    ```
2. Configure with CMake against VTK. Point `VTK_DIR` to either the build tree location or `/path/to/vtk/install/lib/cmake/vtk-x.y` where x.y is the major and minor version of VTK. Ex: `vtk-9.2`.
    ```sh
    cmake -S . -B build -DVTK_DIR=/path/to/vtk/install/lib/cmake/vtk-x.y
    ```
3. Build
    ```sh
    cmake --build build
    ```
4. Run unit tests. 
    ```sh
    ctest --test-dir build
    ```
    *Note*: If you're linking against shared libraries on windows, do not forget to add the location of VTK binaries to the PATH environment variable.
