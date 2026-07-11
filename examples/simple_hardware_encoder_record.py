"""Record a render window to an H.264 file using a hardware video encoder.

The concrete backend is chosen by vtkEncoderFactory from a preference string rather than
hard-coded, so the same example drives Apple's VideoToolbox on macOS and NVENC on NVIDIA
GPUs (whichever hardware H.264 encoder is available on this machine).

Tip: After running this, quit it and play back the recording with:
    ffplay recording.h264
The encoder emits an H.264 Annex B stream.
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
from vtk_streaming.vtkStreamingEncode import vtkEncoderFactory, vtkVideoEncoder
from vtk_streaming.vtkStreamingOpenGL2 import vtkOpenGLVideoFrame

# Ask the encoder factory for a hardware H.264 encoder. On macOS this resolves to the
# VideoToolbox backend, on NVIDIA GPUs to NVENC; the factory keeps the backend choice out of
# this script. (This interim helper stands in for VTK 9.7's vtkObjectFactory::SetPreferences
# + New().)
if not vtkEncoderFactory.CheckAvailability(VTKVC_H264):
    raise SystemExit("No hardware H.264 encoder is available on this machine.")

width, height = 640, 480  # codecs prefer sizes aligned to %4 or %8

# The scene that gets encoded.
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
scene_window.SetWindowName("Input scene (hardware H.264 encode)")
scene_window.AddRenderer(renderer)
scene_window.SetSize(width, height)
scene_window.SetPosition(50, 50)

interactor = vtkRenderWindowInteractor()
interactor.SetRenderWindow(scene_window)
interactor.Initialize()
scene_window.Render()

vtkEncoderFactory.SetPreferences("Codec=H264;Hardware=true")
encoder = vtkEncoderFactory.CreateEncoder()
if encoder is None:
    raise SystemExit("The encoder factory could not create a hardware H.264 encoder.")
print(f"Selected encoder backend: {encoder.GetClassName()}")
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

print("Interact with the window; frames are encoded to ./recording.h264.")
print("Press 'q' or 'e' in the window to quit.")
interactor.Start()

encoder.Drain()
encoder.Shutdown()
