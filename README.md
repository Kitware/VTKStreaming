# VTKStreaming

This experimental module provides classes to encode and stream frames from a VTK OpenGL render window. A number of codecs are supported. Please see [support](support.md) document.

# Build

Requires a VTK installation. Please point `VTK_DIR` cmake variable to a VTK root directory.

1. Clone the repository, run `Utilities/SetupForDevelopment.sh` if you plan on contributing.
2. `$ mkdir build && cd build`
3. `$ cmake ..`
4. `$ cmake --build .`
