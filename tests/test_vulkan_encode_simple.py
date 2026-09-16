"""Exercise the Vulkan Video H.264 encoder with NV12 input.

Moving color bars are generated in memory and uploaded straight to the
vtkOpenGLVideoFrame. There is no Vulkan decoder module, so instead of an image
round-trip the test checks that the emitted bitstream is a plausible H.264
Annex B stream and not all zeros. A resize case covers the session rebuild.
"""

import re

import pytest

import vtkmodules.vtkRenderingOpenGL2  # noqa: F401 (register the OpenGL factory)
from vtkmodules.util.misc import calldata_type
from vtkmodules.util.numpy_support import vtk_to_numpy
from vtkmodules.util.vtkConstants import VTK_OBJECT

from vtk_streaming.vtkStreamingCore import VTKPF_NV12, VTKVC_H264, vtkRawVideoFrame
from vtk_streaming.vtkStreamingEncode import vtkVideoEncoder
from vtk_streaming.vtkStreamingOpenGL2 import vtkOpenGLVideoFrame

from tests import video_test_utils

pytestmark = [
    video_test_utils.requires_rendering(),
    video_test_utils.requires_vulkan_encode(),
]

# Import guarded behind the skip marker: the module only exists on Linux.
vtkVulkanEncoder = pytest.importorskip(
    "vtk_streaming.vtkStreamingVulkanEncode"
).vtkVulkanEncoder

WIDTH = 320
HEIGHT = 240
NUM_FRAMES = 30


def _make_picture(window, width, height):
    picture = vtkOpenGLVideoFrame()
    picture.SetContext(window)
    picture.SetWidth(width)
    picture.SetHeight(height)
    picture.SetPixelFormat(VTKPF_NV12)
    picture.SetSliceOrderType(vtkRawVideoFrame.TopDown)
    picture.ComputeDefaultStrides()
    picture.AllocateDataStore()
    return picture


def _push_frames(encoder, window, picture, width, height, num_frames):
    est_size = vtkRawVideoFrame.GetEstimatedSize(width, height, VTKPF_NV12)
    for shift in range(num_frames):
        frame_bytes = video_test_utils.nv12_frame_bytes(
            video_test_utils.generate_rgba32_color_bars(width, height, shift)
        )
        assert len(frame_bytes) == est_size  # plane offsets rely on this
        video_test_utils.upload_nv12_frame(picture, frame_bytes, width, height)
        picture.Render(window)
        encoder.Encode(picture)


def _assert_annex_b_stream(packets, num_frames):
    assert len(packets) == num_frames
    assert all(len(packet) > 10 for packet in packets)
    assert packets[0][:4] == b"\x00\x00\x00\x01" or packets[0][:3] == b"\x00\x00\x01"
    assert all(any(packet) for packet in packets)


def _assert_codec_names(codec_names):
    assert codec_names
    assert all(re.fullmatch(r"avc1\.[0-9A-F]{6}", name) for name in codec_names), codec_names


def _make_encoder(window, width, height, packets, codec_names=None):
    @calldata_type(VTK_OBJECT)
    def receive_packet(_encoder, _event, packet):
        packets.append(vtk_to_numpy(packet.GetData()).tobytes())
        if codec_names is not None:
            codec_names.append(packet.GetCodecLongName())

    encoder = vtkVulkanEncoder()
    encoder.AddObserver(vtkVideoEncoder.EncodedVideoChunkEvent, receive_packet)
    encoder.SetGraphicsContext(window)
    encoder.SetWidth(width)
    encoder.SetHeight(height)
    encoder.SetCodec(VTKVC_H264)
    encoder.SetInputPixelFormat(VTKPF_NV12)
    return encoder


def test_vulkan_encode_simple():
    input_window = video_test_utils.make_offscreen_window(WIDTH, HEIGHT)
    packets = []
    codec_names = []
    encoder = _make_encoder(input_window, WIDTH, HEIGHT, packets, codec_names)
    picture = _make_picture(input_window, WIDTH, HEIGHT)

    _push_frames(encoder, input_window, picture, WIDTH, HEIGHT, NUM_FRAMES)
    encoder.Drain()

    _assert_annex_b_stream(packets, NUM_FRAMES)
    _assert_codec_names(codec_names)
    encoder.Shutdown()


def test_vulkan_encode_resize():
    input_window = video_test_utils.make_offscreen_window(WIDTH, HEIGHT)
    packets = []
    encoder = _make_encoder(input_window, WIDTH, HEIGHT, packets)

    picture = _make_picture(input_window, WIDTH, HEIGHT)
    _push_frames(encoder, input_window, picture, WIDTH, HEIGHT, 5)
    first_batch = len(packets)

    # A frame of a new size tears the session down and rebuilds it.
    big_w, big_h = 640, 480
    input_window.SetSize(big_w, big_h)
    encoder.SetWidth(big_w)
    encoder.SetHeight(big_h)
    picture = _make_picture(input_window, big_w, big_h)
    _push_frames(encoder, input_window, picture, big_w, big_h, 5)
    encoder.Drain()

    _assert_annex_b_stream(packets, 10)
    # The first packet after the resize must be a key frame carrying new parameter sets.
    resized = packets[first_batch]
    assert resized[:4] == b"\x00\x00\x00\x01" or resized[:3] == b"\x00\x00\x01"
    encoder.Shutdown()
