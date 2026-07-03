"""Exercise display encoding with the libvpx VP9 encoder.

A scene with a cylinder is captured from the
render window in IYUV or NV12 and encoded. The output
of encoder must have a valid h.264 stream for this test to pass.
"""

import pytest

import vtkmodules.vtkRenderingOpenGL2  # noqa: F401 (register the OpenGL factory)
from vtkmodules.util.misc import calldata_type
from vtkmodules.util.numpy_support import vtk_to_numpy
from vtkmodules.util.vtkConstants import VTK_OBJECT
from vtkmodules.vtkFiltersSources import vtkCylinderSource
from vtkmodules.vtkRenderingCore import vtkActor, vtkPolyDataMapper, vtkRenderer

from vtk_streaming.vtkStreamingCore import (  # noqa: E402
    VTKPF_IYUV,
    VTKPF_NV12,
    VTKVC_H264,
    vtkRawVideoFrame,
)
from vtk_streaming.vtkStreamingEncode import vtkVideoEncoder  # noqa: E402
from vtk_streaming.vtkStreamingOpenGL2 import vtkOpenGLVideoFrame  # noqa: E402
from vtk_streaming.vtkStreamingNvEncode import vtkNvEncoderGL  # noqa: E402

from tests import video_test_utils  # noqa: E402

pytestmark = [
    video_test_utils.requires_rendering(),
    video_test_utils.requires_nvenc(),
]

WIDTH = 641
HEIGHT = 953
NUM_FRAMES = 30
# The captured scene goes through RGB->YUV 4:2:0 capture, VP9 quantization and
# YUV->RGB display, so allow more slack than the file-based round-trip.
THRESHOLD = 0.1


def _make_cylinder_scene():
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
    renderer.SetBackground(26 / 255, 51 / 255, 102 / 255)
    return renderer


@pytest.mark.parametrize("pixel_format", [VTKPF_IYUV, VTKPF_NV12], ids=["iyuv", "nv12"])
def test_nv_encode_decode_render_window_capture(pixel_format, tmp_path):
    scene_window = video_test_utils.make_offscreen_window(WIDTH, HEIGHT)
    renderer = _make_cylinder_scene()
    scene_window.AddRenderer(renderer)
    scene_window.Render()

    packets = []

    @calldata_type(VTK_OBJECT)
    def receive_packet(_encoder, _event, packet):
        packets.append(vtk_to_numpy(packet.GetData()).tobytes())

    encoder = vtkNvEncoderGL()
    encoder.AddObserver(vtkVideoEncoder.EncodedVideoChunkEvent, receive_packet)
    encoder.SetGraphicsContext(scene_window)
    encoder.SetCodec(VTKVC_H264)
    encoder.SetWidth(WIDTH)
    encoder.SetHeight(HEIGHT)
    encoder.SetInputPixelFormat(pixel_format)

    picture = vtkOpenGLVideoFrame()
    picture.SetContext(scene_window)
    picture.SetWidth(WIDTH)
    picture.SetHeight(HEIGHT)
    picture.SetPixelFormat(pixel_format)
    picture.SetSliceOrderType(vtkRawVideoFrame.TopDown)
    picture.AllocateDataStore()

    for _frame_id in range(NUM_FRAMES):
        renderer.GetActiveCamera().Azimuth(2.0)
        scene_window.Render()
        picture.Capture(scene_window)
        encoder.Encode(picture)

    # drain needs an OpenGL context so it can release the resources.
    encoder.Drain()

    assert len(packets) == NUM_FRAMES
    assert all(len(packet) > 10 for packet in packets[1:])
    # A valid H.264 Annex B stream opens with a start code, and every packet
    # must carry real payload rather than zero filler.
    # Both of these are valid stream openings.
    assert packets[0][:4] == b"\x00\x00\x00\x01" or packets[0][:3] == b"\x00\x00\x01"
    assert all(any(packet) for packet in packets)

    encoder.Shutdown()
