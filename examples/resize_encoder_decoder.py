"""VP9 encode/decode round-trip that survives window resizes.

Same side-by-side layout as examples/simple_encoder_decoder.py: the left
window renders a spinning cylinder, every finished render (vtkCommand::
EndEvent) is captured and VP9-encoded, and each packet is decoded and
displayed in the right window.

The addition is resize handling, and it is almost free: whenever the scene
window's size changes, the capture frame is rebuilt at the new size — the
next Encode then makes vtkVideoEncoder tear down and rebuild its context
from the new frame dimensions, and the decoder follows the packet dimensions
on its own. Only the output window needs an explicit SetSize to match.

Drag-resize the left window, or just wait: a timer cycles it through a few
sizes automatically.
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
    VTKVC_VP9,
    vtkCompressedVideoPacket,
    vtkRawVideoFrame,
)
from vtk_streaming.vtkStreamingDecode import vtkVideoDecoder
from vtk_streaming.vtkStreamingEncode import vtkVideoEncoder
from vtk_streaming.vtkStreamingOpenGL2 import vtkOpenGLVideoFrame
from vtk_streaming.vtkStreamingVpxDecode import vtkVpxDecoder
from vtk_streaming.vtkStreamingVpxEncode import vtkVpxEncoder

# The automatic size cycle; codecs prefer sizes aligned to %4 or %8.
SIZES = [(640, 480), (800, 600), (480, 360)]
TICKS_PER_SIZE = 120  # timer ticks between automatic resizes (~4 s at 33 ms)

width, height = SIZES[0]

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
# window. Normalized viewport coordinates keep it in the corner across resizes.
frame_text = vtkTextActor()
frame_text.SetInput(current_time_text())
frame_text.GetTextProperty().SetFontSize(18)
frame_text.GetTextProperty().SetJustificationToRight()
frame_text.GetTextProperty().SetVerticalJustificationToTop()
frame_text.GetPositionCoordinate().SetCoordinateSystemToNormalizedViewport()
frame_text.GetPositionCoordinate().SetValue(0.98, 0.98)
renderer.AddActor2D(frame_text)

scene_window = vtkRenderWindow()
scene_window.SetWindowName("Input scene (VP9 encode)")
scene_window.AddRenderer(renderer)
scene_window.SetSize(width, height)
scene_window.SetPosition(50, 50)

interactor = vtkRenderWindowInteractor()
interactor.SetRenderWindow(scene_window)
interactor.Initialize()
scene_window.Render()

# Right window: displays whatever comes out of the decoder.
decoded_window = vtkRenderWindow()
decoded_window.SetWindowName("Decoded output (VP9 decode)")
decoded_window.SetSize(width, height)
decoded_window.SetPosition(50 + max(w for w, _ in SIZES) + 20, 50)
decoded_window.Render()  # initialize its OpenGL context before first use

decoder = vtkVpxDecoder()
decoder.SetGraphicsContext(decoded_window)


@calldata_type(VTK_OBJECT)
def display_frame(_decoder: vtkVideoDecoder, _event: int, frame: vtkOpenGLVideoFrame):
    # frame.Render presents into the window (it calls window->Frame() itself).
    frame.Render(decoded_window)


decoder.AddObserver(vtkVideoDecoder.DecodedVideoFrameEvent, display_frame)

encoder = vtkVpxEncoder()
encoder.SetGraphicsContext(scene_window)
encoder.SetCodec(VTKVC_VP9)
encoder.SetWidth(width)
encoder.SetHeight(height)
encoder.SetInputPixelFormat(VTKPF_IYUV)


@calldata_type(VTK_OBJECT)
def receive_packet(
    _encoder: vtkVideoEncoder, _event: int, packet: vtkCompressedVideoPacket
):
    # In a real application this bitstream would travel over the network;
    # here it goes straight to the decoder. Each packet carries its display
    # dimensions, which is how the decoder picks up a resize.
    decoder.Decode(packet)


encoder.AddObserver(vtkVideoEncoder.EncodedVideoChunkEvent, receive_packet)


def make_picture(window, picture_width, picture_height):
    picture = vtkOpenGLVideoFrame()
    picture.SetContext(window)
    picture.SetWidth(picture_width)
    picture.SetHeight(picture_height)
    picture.SetPixelFormat(VTKPF_IYUV)
    picture.SetSliceOrderType(vtkRawVideoFrame.TopDown)
    picture.AllocateDataStore()
    return picture


picture = make_picture(scene_window, width, height)


def update_time_text(_window: vtkRenderWindow, _event: int):
    frame_text.SetInput(current_time_text())


def encode_frame(window: vtkRenderWindow, _event: int):
    global picture
    size = tuple(window.GetSize())
    if size != (picture.GetWidth(), picture.GetHeight()):
        # Rebuild the capture frame; the first Encode at the new dimensions
        # makes the encoder rebuild its context, and the decoder follows the
        # packet dimensions. Only the output window needs explicit resizing.
        print(f"resized to {size[0]}x{size[1]}")
        picture = make_picture(window, *size)
        decoded_window.SetSize(*size)
    picture.Capture(window)
    encoder.Encode(picture)  # fires EncodedVideoChunkEvent per packet
    # Decoding rendered into the other window; hand the context back.
    window.MakeCurrent()


# StartEvent fires at the start of every vtkRenderWindow::Render, so the
# timestamp is current in the frame about to be drawn and encoded;
# EndEvent fires at the end.
scene_window.AddObserver(vtkCommand.StartEvent, update_time_text)
scene_window.AddObserver(vtkCommand.EndEvent, encode_frame)

tick = 0


def spin_and_cycle_size(_interactor: vtkRenderWindowInteractor, _event: int):
    global tick
    tick += 1
    renderer.GetActiveCamera().Azimuth(1.0)
    if tick % TICKS_PER_SIZE == 0:
        scene_window.SetSize(*SIZES[(tick // TICKS_PER_SIZE) % len(SIZES)])
    scene_window.Render()


interactor.AddObserver(vtkCommand.TimerEvent, spin_and_cycle_size)
interactor.CreateRepeatingTimer(33)

print("Drag-resize the left window, or wait for the automatic size cycle.")
print("Press 'q' or 'e' in the left window to quit.")
interactor.Start()

encoder.Drain()
encoder.Shutdown()
# Note: do not call decoder.Drain(); vtkVpxDecoder::SendEOS forwards a null
# packet that DecodeInternal dereferences. Shutdown is safe.
decoder.Shutdown()
