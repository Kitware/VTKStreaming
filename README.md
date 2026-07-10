# VTKStreaming

This module provides classes to encode and stream frames
from a VTK OpenGL render window using video codecs. 
It supports video encoding with VP9 (through [libvpx](https://chromium.googlesource.com/webm/libvpx/))
and H.264/H.265 through hardware encoders: [NVENC](https://developer.nvidia.com/nvidia-video-codec-sdk/download)
on NVIDIA GPUs, and [VideoToolbox](https://developer.apple.com/documentation/videotoolbox)
on macOS (Apple Silicon and Intel). 

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

There are two build flows:

- **Development build**: an editable install into a local virtual environment,
  built *without* build isolation. Rebuilds are fast and the generated
  `compile_commands.json` stays valid, so clangd/IDE tooling works.
- **Release build**: [cibuildwheel](https://cibuildwheel.pypa.io/) produces the
  exact same distributable wheel as CI, in an isolated environment.

Requirements (both flows):
- Linux: A C++ compiler (GCC 11.4+ or any other compiler supported by VTK.)
- Windows: MSVC (Visual Studio Build Tools). (ensure visual studio environment is initialized)
- macOS: Xcode command line tools. (`xcode-select --install` should have completed successfully)

We recommend [uv](https://docs.astral.sh/uv/); it reads the `vtk-sdk` package
index from `pyproject.toml`, so no extra index flags are needed.

### Development build

The editable install runs with `--no-build-isolation`, so the build
requirements (`scikit-build-core`, `vtk-sdk`, `vtk-sdk-python-wheel-helper`)
must be present in the environment first, the `dev` dependency group installs
them along with the test dependencies.

Linux/macOS:

```sh
uv venv -p 3.13
. .venv/bin/activate
uv pip install --group dev
uv pip install -e . --no-build-isolation
```

Windows (open a PowerShell with MSVC initialized, e.g. Visual Studio Developer PowerShell):

```powershell
uv venv -p 3.13
.venv\Scripts\Activate.ps1
uv pip install --group dev
uv pip install -e . --no-build-isolation
```

After changing C++ sources, rebuild by re-running the editable install:

```sh
uv pip install -e . --no-build-isolation
```

The CMake build tree persists under `build/<wheel_tag>/`, which keeps generated
module headers on disk and `compile_commands.json` valid.

Run the tests with:

```sh
pytest tests/ -v
```

### Release build (reproduce CI artifacts)

Wheels are built with [cibuildwheel](https://cibuildwheel.pypa.io/) in an
isolated environment; all configuration lives in `[tool.cibuildwheel]` in
`pyproject.toml`.

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
3. [examples/simple_hardware_encoder_record.py](./examples/simple_hardware_encoder_record.py) - Record a render window for later playback using whichever hardware H.264 encoder `vtkEncoderFactory` selects (Apple VideoToolbox on macOS, NVENC on NVIDIA GPUs). This needs `ffplay` to playback the .h264 file.

## Getting help

- For issues with the API, usage or bugs in VTKStreaming libraries,
[please report them on the original repository](https://gitlab.kitware.com/async/vtkstreaming/issues).
- For issues with wheel installation, supported Python and VTK versions or Python-side issues,
[please report them on the GitHub Fork](https://github.com/Kitware/VTKStreaming/issues).
