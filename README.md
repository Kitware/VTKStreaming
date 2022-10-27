# VTKStreaming

This experimental module provides classes to encode and stream frames
from a VTK OpenGL render window. Supports video encoding with VP9 (through [libvpx](https://chromium.googlesource.com/webm/libvpx/))
and H.264/H.265 (through [NVENC](https://developer.nvidia.com/nvidia-video-codec-sdk/download)).

# Requirements

If you plan on developing, `git-lfs` is needed to download the test images, videos.
Please point `VTK_DIR` cmake variable to a VTK root directory. You will need `nasm` and `perl`
to compile the `libvpx` third-party library.
## Common:
Acquire `nasm`. You may download binaries for linux, windows and mac from [nasm.us](https://nasm.us).
## Linux and MacOS
Acquire `perl` from your package manager.
## Windows:
[Strawberry perl](https://strawberryperl.com/) works on windows.

# Instructions
1. Clone the repository, run `Utilities/SetupForDevelopment.sh` if necessary.
2. `$ mkdir build && cd build`
3. `$ cmake ..`
4. `$ cmake --build .`
5. `$ ctest`
