"""Exercise display encoding with the libvpx VP9 encoder.

a scene with a cylinder is captured from the
render window in IYUV or NV12 and encoded. Round-trip upgrade: every encoded
packet is fed to a vtkVpxDecoder whose decoded frames are rendered into a
second render window, and the two windows are compared with
vtkTesting.RegressionTest.
"""

import pytest

import vtkmodules.vtkRenderingOpenGL2  # noqa: F401 (register the OpenGL factory)
from vtkmodules.util.misc import calldata_type
from vtkmodules.util.vtkConstants import VTK_OBJECT
from vtkmodules.vtkFiltersSources import vtkCylinderSource
from vtkmodules.vtkRenderingCore import vtkActor, vtkPolyDataMapper, vtkRenderer
from vtkmodules.vtkTestingRendering import vtkTesting

pytest.importorskip(
    "vtk_streaming.vtkStreamingVpxDecode",
    reason="installed vtk_streaming wheel was built without the VpxDecode module",
)

from vtk_streaming.vtkStreamingCore import (  # noqa: E402
    VTKPF_IYUV,
    VTKPF_NV12,
    VTKVC_VP9,
    vtkRawVideoFrame,
)
from vtk_streaming.vtkStreamingEncode import vtkVideoEncoder  # noqa: E402
from vtk_streaming.vtkStreamingOpenGL2 import vtkOpenGLVideoFrame  # noqa: E402
from vtk_streaming.vtkStreamingVpxEncode import vtkVpxEncoder  # noqa: E402

from tests import video_test_utils  # noqa: E402

pytestmark = video_test_utils.requires_rendering()

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


@pytest.mark.parametrize(
    "pixel_format", [VTKPF_IYUV, VTKPF_NV12], ids=["iyuv", "nv12"]
)
def test_vpx_encode_decode_render_window_capture(pixel_format, tmp_path):
    scene_window = video_test_utils.make_offscreen_window(WIDTH, HEIGHT)
    renderer = _make_cylinder_scene()
    scene_window.AddRenderer(renderer)
    scene_window.Render()

    decoded_window = video_test_utils.make_offscreen_window(WIDTH, HEIGHT)
    decoder, decoded_frame_sizes = video_test_utils.make_decode_render_loop(
        decoded_window
    )

    packet_sizes = []

    @calldata_type(VTK_OBJECT)
    def receive_packet(_encoder, _event, packet):
        packet_sizes.append(packet.GetSize())
        decoder.Decode(packet)

    encoder = vtkVpxEncoder()
    encoder.AddObserver(vtkVideoEncoder.EncodedVideoChunkEvent, receive_packet)
    encoder.SetGraphicsContext(scene_window)
    encoder.SetCodec(VTKVC_VP9)
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

    assert len(packet_sizes) == NUM_FRAMES
    assert all(size > 10 for size in packet_sizes[1:])
    assert decoded_frame_sizes == [(WIDTH, HEIGHT)] * len(packet_sizes)

    # The scene window shows the last rendered frame, the decoded window the
    # last round-tripped frame; they must match up to codec loss.
    result = video_test_utils.regression_compare_windows(
        scene_window, decoded_window, THRESHOLD, tmp_path
    )
    assert result == vtkTesting.PASSED

    encoder.Shutdown()
    decoder.Shutdown()
