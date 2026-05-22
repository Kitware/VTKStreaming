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

## Example

```py
from vtkmodules.vtkCommonCore import vtkUnsignedCharArray
from vtkmodules.vtkRenderingCore import vtkRenderer, vtkRenderWindow, vtkRenderWindowInteractor
from vtkmodules.util.numpy_support import vtk_to_numpy
from vtkmodules.util.misc import calldata_type
from vtkmodules.util.vtkConstants import VTK_OBJECT

from vtk_streaming.vtkStreamingEncode import vtkVideoEncoder
from vtk_streaming.vtkStreamingNvEncode import vtkNvEncoderGL
from vtk_streaming.vtkStreamingVpxEncode import vtkVpxEncoder
from vtk_streaming.vtkStreamingOpenGL2 import vtkOpenGLVideoFrame
from vtk_streaming.vtkStreamingCore import VTKVC_H264, VTKVC_H265, VTKVC_VP9, VTKPF_IYUV, vtkCompressedVideoPacket

ren = vtkRenderer()
ren.SetBackground(0.1, 0.2, 0.4)
win = vtkRenderWindow()
win.AddRenderer(ren)
width, height = 641, 953 # size of video frames will likely be aligned to some value such as %4 or %8
win.SetSize(width, height)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(win)
iren.Initialize()
iren.Render()

# Encoder takes the window to get the OpenGL context from it
encoder: vtkVideoEncoder = None
if vtkNvEncoderGL.CheckAvailability():
    encoder = vtkNvEncoderGL()
    encoder.SetCodec(VTKVC_H264)
    print("Using H264 through NVENC")
else:
    encoder = vtkVpxEncoder()
    encoder.SetCodec(VTKVC_VP9)
    print("Using VP9 through libvpx")
encoder.SetGraphicsContext(win)
encoder.SetWidth(width) # to handle in resize event!
encoder.SetHeight(height)
encoder.SetInputPixelFormat(VTKPF_IYUV)

# This is used to copy the framebuffer of the window to a NVENC shared-texture
picture = vtkOpenGLVideoFrame()
picture.SetContext(win)
picture.SetWidth(width) # to handle in resize event!
picture.SetHeight(height)
picture.SetPixelFormat(VTKPF_IYUV)
picture.AllocateDataStore()

# You will receive video packets through this callback
@calldata_type(VTK_OBJECT)
def receive_data(_obj: vtkVideoEncoder, _ev: int, data: vtkCompressedVideoPacket):
    frame_data: vtkUnsignedCharArray = data.GetData()
    raw_bytes = vtk_to_numpy(frame_data).tobytes() # do something with it
encoder.AddObserver(vtkVideoEncoder.EncodedVideoChunkEvent, receive_data)

# We have logic to insert in rendering loop,
# Depending on the application and used GUI etc, this might differ.
while True:
    # Render current frame
    iren.ProcessEvents()
    iren.Render()
    # Capture last framebuffer
    picture.Capture(win)
    # Encode using choosen backend, this may invoke EncodedVideoChunkEvent any number of times
    encoder.Encode(picture)

# In a real application this should be called on exit
# this example is simply killed by user with ctrl+c
enc.Drain()
enc.Shutdown()
```

## Getting help

- For issues with the API, usage or bugs in VTKStreaming libraries,
[please report them on the original repository](https://gitlab.kitware.com/async/vtkstreaming/issues).
- For issues with wheel installation, supported Python and VTK versions or Python-side issues,
[please report them on the GitHub Fork](https://github.com/Kitware/VTKStreaming/issues).
