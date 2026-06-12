"""Exercise the libvpx VP9 encoder with IYUV and NV12 raw frame inputs.

moving color bars are generated in memory instead and uploaded straight to the vtkOpenGLVideoFrame.
Round-trip: every encoded packet is fed to a vtkVpxDecoder whose decoded frames are
rendered into a second render window, and the two windows are compared with
vtkTesting.RegressionTest.
"""

import pytest

import vtkmodules.vtkRenderingOpenGL2  # noqa: F401 (register the OpenGL factory)
from vtkmodules.util.misc import calldata_type
from vtkmodules.util.vtkConstants import VTK_OBJECT
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

WIDTH = 320
HEIGHT = 240
NUM_FRAMES = 30
# Both windows show the same YUV data modulo VP9 quantization, so the tight
# default threshold is enough; bump towards 0.1 if encoding pixelates more.
THRESHOLD = 0.05


_FORMATS = {
    "iyuv": (
        VTKPF_IYUV,
        video_test_utils.iyuv_frame_bytes,
        video_test_utils.upload_iyuv_frame,
    ),
    "nv12": (
        VTKPF_NV12,
        video_test_utils.nv12_frame_bytes,
        video_test_utils.upload_nv12_frame,
    ),
}


@pytest.mark.parametrize("format_name", sorted(_FORMATS))
def test_vpx_encode_decode_simple(format_name, tmp_path):
    pixel_format, pack_frame, upload = _FORMATS[format_name]

    input_window = video_test_utils.make_offscreen_window(WIDTH, HEIGHT)
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
    encoder.SetGraphicsContext(input_window)
    encoder.SetWidth(WIDTH)
    encoder.SetHeight(HEIGHT)
    encoder.SetCodec(VTKVC_VP9)
    encoder.SetInputPixelFormat(pixel_format)

    picture = vtkOpenGLVideoFrame()
    picture.SetContext(input_window)
    picture.SetWidth(WIDTH)
    picture.SetHeight(HEIGHT)
    picture.SetPixelFormat(pixel_format)
    picture.SetSliceOrderType(vtkRawVideoFrame.TopDown)
    picture.ComputeDefaultStrides()
    picture.AllocateDataStore()

    est_size = vtkRawVideoFrame.GetEstimatedSize(WIDTH, HEIGHT, pixel_format)

    for shift in range(NUM_FRAMES):
        frame_bytes = pack_frame(
            video_test_utils.generate_rgba32_color_bars(WIDTH, HEIGHT, shift)
        )
        assert len(frame_bytes) == est_size  # plane offsets below rely on this
        upload(picture, frame_bytes, WIDTH, HEIGHT)
        picture.Render(input_window)
        encoder.Encode(picture)
    # drain out remaining packets
    encoder.Drain()

    assert len(packet_sizes) == NUM_FRAMES
    assert all(size > 10 for size in packet_sizes[1:])
    assert decoded_frame_sizes == [(WIDTH, HEIGHT)] * len(packet_sizes)

    # The input window shows the last uploaded frame, the decoded window the
    # last round-tripped frame; they must match up to codec loss.
    result = video_test_utils.regression_compare_windows(
        input_window, decoded_window, THRESHOLD, tmp_path
    )
    assert result == vtkTesting.PASSED

    encoder.Shutdown()
    decoder.Shutdown()
