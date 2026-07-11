"""Exercise the VideoToolbox H.264 encoder with IYUV, NV12 and RGBA32 inputs.

Moving color bars are generated in memory and uploaded straight to the
vtkOpenGLVideoFrame. There is no VideoToolbox decoder module, so instead of an
image round-trip the test checks that the emitted bitstream is a plausible
H.264 Annex B stream and not all zeros.
"""

import pytest

import vtkmodules.vtkRenderingOpenGL2  # noqa: F401 (register the OpenGL factory)
from vtkmodules.util.misc import calldata_type
from vtkmodules.util.numpy_support import vtk_to_numpy
from vtkmodules.util.vtkConstants import VTK_OBJECT

from vtk_streaming.vtkStreamingCore import (
    VTKPF_IYUV,
    VTKPF_NV12,
    VTKPF_RGBA32,
    VTKVC_H264,
    vtkRawVideoFrame,
)
from vtk_streaming.vtkStreamingEncode import vtkVideoEncoder
from vtk_streaming.vtkStreamingOpenGL2 import vtkOpenGLVideoFrame

from tests import video_test_utils

pytestmark = [
    video_test_utils.requires_rendering(),
    video_test_utils.requires_videotoolbox(),
]

# Import guarded behind the skip marker: the module only exists on macOS.
vtkVideoToolboxEncoder = pytest.importorskip(
    "vtk_streaming.vtkStreamingVTEncode"
).vtkVideoToolboxEncoder

WIDTH = 320
HEIGHT = 240
NUM_FRAMES = 30

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
    "rgba32": (
        VTKPF_RGBA32,
        video_test_utils.rgba32_frame_bytes,
        video_test_utils.upload_rgba32_frame,
    ),
}


@pytest.mark.parametrize("format_name", sorted(_FORMATS))
def test_videotoolbox_encode_simple(format_name):
    pixel_format, pack_frame, upload = _FORMATS[format_name]

    input_window = video_test_utils.make_offscreen_window(WIDTH, HEIGHT)

    packets = []

    @calldata_type(VTK_OBJECT)
    def receive_packet(_encoder, _event, packet):
        packets.append(vtk_to_numpy(packet.GetData()).tobytes())

    encoder = vtkVideoToolboxEncoder()
    encoder.AddObserver(vtkVideoEncoder.EncodedVideoChunkEvent, receive_packet)
    encoder.SetGraphicsContext(input_window)
    encoder.SetWidth(WIDTH)
    encoder.SetHeight(HEIGHT)
    encoder.SetCodec(VTKVC_H264)
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
        assert len(frame_bytes) == est_size  # plane offsets rely on this
        upload(picture, frame_bytes, WIDTH, HEIGHT)
        picture.Render(input_window)
        encoder.Encode(picture)
    # drain out remaining packets
    encoder.Drain()

    assert len(packets) == NUM_FRAMES
    assert all(len(packet) > 10 for packet in packets[1:])
    # A valid H.264 Annex B stream opens with a start code, and every packet
    # must carry real payload rather than zero filler.
    assert packets[0][:4] == b"\x00\x00\x00\x01" or packets[0][:3] == b"\x00\x00\x01"
    assert all(any(packet) for packet in packets)

    encoder.Shutdown()
