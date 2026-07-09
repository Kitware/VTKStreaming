"""Live VP9 encode/decode round-trip with two render windows side by side.

This uses an NVENC encoder to record the render window to a .h264 file.

Tip: After running this, quit it and playback the recording with `ffplay recording.h264`
"""

from datetime import datetime

import vtkmodules.vtkRenderingOpenGL2  # noqa: F401 (register the OpenGL factory)
import vtkmodules.vtkInteractionStyle  # noqa: F401 (register interactor styles)
from vtkmodules.util.misc import calldata_type
from vtkmodules.util.vtkConstants import VTK_OBJECT
from vtkmodules.vtkCommonCore import vtkCommand
from vtkmodules.vtkFiltersSources import vtkCylinderSource
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkPolyDataMapper,
    vtkRenderer,
    vtkRenderWindow,
    vtkRenderWindowInteractor,
    vtkTextActor,
)

from vtk_streaming.vtkStreamingCore import (
    VTKPF_IYUV,
    VTKVC_H264,
    vtkCompressedVideoPacket,
    vtkRawVideoFrame,
)
from vtk_streaming.vtkStreamingEncode import vtkVideoEncoder
from vtk_streaming.vtkStreamingOpenGL2 import vtkOpenGLVideoFrame
from vtk_streaming.vtkStreamingNvEncode import vtkNvEncoderGL

width, height = 640, 480  # codecs prefer sizes aligned to %4 or %8

# Left window: the scene that gets encoded.
cylinder = vtkCylinderSource()
mapper = vtkPolyDataMapper()
mapper.SetInputConnection(cylinder.GetOutputPort())
actor = vtkActor()
actor.SetMapper(mapper)
actor.GetProperty().SetColor(255 / 255, 99 / 255, 71 / 255)  # tomato
actor.RotateX(30.0)
actor.RotateY(-45.0)
renderer = vtkRenderer()
renderer.AddActor(actor)
renderer.SetBackground(0.1, 0.2, 0.4)


def current_time_text() -> str:
    now = datetime.now()
    return f"{now:%H:%M:%S}.{now.microsecond // 1000:03d}"


# Time readout (HH:MM:SS.ms), anchored to the top right corner of the scene
# window.
frame_text = vtkTextActor()
frame_text.SetInput(current_time_text())
frame_text.GetTextProperty().SetFontSize(18)
frame_text.GetTextProperty().SetJustificationToRight()
frame_text.GetTextProperty().SetVerticalJustificationToTop()
frame_text.GetPositionCoordinate().SetCoordinateSystemToNormalizedViewport()
frame_text.GetPositionCoordinate().SetValue(0.98, 0.98)
renderer.AddViewProp(frame_text)

scene_window = vtkRenderWindow()
scene_window.SetWindowName("Input scene (VP9 encode)")
scene_window.AddRenderer(renderer)
scene_window.SetSize(width, height)
scene_window.SetPosition(50, 50)

interactor = vtkRenderWindowInteractor()
interactor.SetRenderWindow(scene_window)
interactor.Initialize()
scene_window.Render()

encoder = vtkNvEncoderGL()
encoder.SetGraphicsContext(scene_window)
encoder.SetCodec(VTKVC_H264)
encoder.SetWidth(width)
encoder.SetHeight(height)
encoder.SetInputPixelFormat(VTKPF_IYUV)


@calldata_type(VTK_OBJECT)
def receive_packet(
    _encoder: vtkVideoEncoder, _event: int, packet: vtkCompressedVideoPacket
):
    with open("./recording.h264", mode="+ab") as f:
        f.write(bytes(memoryview(packet.GetData())))


encoder.AddObserver(vtkVideoEncoder.EncodedVideoChunkEvent, receive_packet)


# The capture frame; its size must match the scene window's framebuffer.
picture = vtkOpenGLVideoFrame()
picture.SetContext(scene_window)
picture.SetWidth(width)
picture.SetHeight(height)
picture.SetPixelFormat(VTKPF_IYUV)
picture.SetSliceOrderType(vtkRawVideoFrame.TopDown)
picture.AllocateDataStore()


def update_time_text(_window: vtkRenderWindow, _event: int):
    frame_text.SetInput(current_time_text())


def encode_frame(window: vtkRenderWindow, _event: int):
    picture.Capture(window)
    encoder.Encode(picture)  # fires EncodedVideoChunkEvent per packet


# StartEvent fires at the start of every vtkRenderWindow::Render, so the
# timestamp is current in the frame about to be drawn and encoded;
# EndEvent fires at the end.
scene_window.AddObserver(vtkCommand.StartEvent, update_time_text)
scene_window.AddObserver(vtkCommand.EndEvent, encode_frame)


def spin(_interactor: vtkRenderWindowInteractor, _event: int):
    renderer.GetActiveCamera().Azimuth(1.0)
    scene_window.Render()


interactor.AddObserver(vtkCommand.TimerEvent, spin)
interactor.CreateRepeatingTimer(33)

print("Interact with the left window; the right window shows the decoded stream.")
print("Press 'q' or 'e' in the left window to quit.")
interactor.Start()

encoder.Drain()
encoder.Shutdown()
