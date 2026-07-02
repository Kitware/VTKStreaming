# VTKStreaming

This module provides classes to encode and stream frames
from a VTK OpenGL render window using video codecs. 
It supports video encoding with VP9 (through [libvpx](https://chromium.googlesource.com/webm/libvpx/))
and H.264/H.265 (through [NVENC](https://developer.nvidia.com/nvidia-video-codec-sdk/download)). 

## Installation

VTKStreaming is available on PyPi on the following platforms:
- Linux x86_64 for python 3.10 to 3.13 included.
- Windows x86_64 for python 3.10 to 3.13 included.
- MacOSX arm64 for python 3.10 to 3.13 included.

It is currently based on VTK 9.6.0.

```sh
pip install vtk-streaming
```

## Building from source

## Build for quick development

Requirements:
- Linux: A C++ compiler (GCC 11.4+ or any other compiler supported by VTK.)
- Windows: MSVC (Visual Studio Build Tools). (ensure visual studio environment is initialized)
- macOS: Xcode command line tools. (`xcode-select --install` should have completed successfully)

### Linux/macOS

```sh
python3 -m venv .venv
. .venv/bin/activate
pip install -e . --extra-index-url https://vtk.org/files/wheel-sdks
```

### Windows

Open a powershell with MSVC initialized (ex: Visual Studio Developer Powershell)

```sh
python3 -m venv .venv
.venv/bin/activate.ps1
pip install -e . --extra-index-url https://vtk.org/files/wheel-sdks
```

## Reproduce CI artifacts
Wheels are built with [cibuildwheel](https://cibuildwheel.pypa.io/).

Requirements:
- Linux: [Docker](https://www.docker.com/) (the build runs in a manylinux container).
- Windows: MSVC (Visual Studio Build Tools). (ensure visual studio environment is initialized)
- macOS: Xcode command line tools. (`xcode-select --install` should have completed successfully)

Build a wheel for one Python/platform target:

```sh
# Linux
uvx cibuildwheel --only cp310-manylinux_x86_64

# Windows
uvx cibuildwheel --only cp310-win_amd64

# macOS
uvx cibuildwheel --only cp310-macosx_arm64
```

`uvx` comes with [uv](https://docs.astral.sh/uv/); alternatively
`pipx run cibuildwheel` or if you use `pip`:

```sh
pip install cibuildwheel

# Linux
cibuildwheel --only cp310-manylinux_x86_64

# Windows
cibuildwheel --only cp310-win_amd64

# macOS
cibuildwheel --only cp310-macosx_arm64
```

Substitute `cp310`/`cp311`/`cp312`/`cp313` to target other Python versions. The wheel is
written to `wheelhouse/` and can be installed directly:

```sh
pip install wheelhouse/vtk_streaming-*.whl
```

## Examples

1. [examples/simple_encoder_decoder.py](./examples/simple_encoder_decoder.py) - Live VP9 encode/decode round-trip with two render windows side by side.
2. [examples/resize_encoder_decoder.py](./examples/resize_encoder_decoder.py) - VP9 encode/decode round-trip that survives window resizes.
3. [examples/simple_nvenc_record.py](./examples/simple_nvenc_record.py) - Record a render window for later playback using NVENC. This needs `ffplay` to playback the .h264 file.

## Getting help

- For issues with the API, usage or bugs in VTKStreaming libraries,
[please report them on the original repository](https://gitlab.kitware.com/async/vtkstreaming/issues).
- For issues with wheel installation, supported Python and VTK versions or Python-side issues,
[please report them on the GitHub Fork](https://github.com/Kitware/VTKStreaming/issues).
