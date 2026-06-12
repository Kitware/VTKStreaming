"""Verify that encoding survives a mid-stream resize.

vtkVideoEncoder::Encode tears down and reinitializes the encoding context
whenever the incoming frame's dimensions change, and vtkVideoDecoder
reinitializes from each packet's DisplayWidth/DisplayHeight. Moving color
bars are pushed through three sizes (grow then shrink). Both tests check
that every packet is stamped with the display dimensions of the size it
was encoded at. The VP9 test additionally round-trips every packet through
a vtkVpxDecoder and image-compares the input and decoded windows at each
size. There is no NVDEC decoder module, so the NVENC test instead checks
that every resize restarts the H.264 stream: the first packet at each size
must carry fresh SPS/PPS and an IDR slice.

Resize handling lives in the encoder/decoder base classes, so one pixel
format (IYUV) is enough; the per-format upload paths are covered by the
push-receive tests.
"""

import pytest

import vtkmodules.vtkRenderingOpenGL2  # noqa: F401 (register the OpenGL factory)
from vtkmodules.util.misc import calldata_type
from vtkmodules.util.numpy_support import vtk_to_numpy
from vtkmodules.util.vtkConstants import VTK_OBJECT
from vtkmodules.vtkTestingRendering import vtkTesting

from vtk_streaming.vtkStreamingCore import (
    VTKPF_IYUV,
    VTKVC_H264,
    VTKVC_VP9,
    vtkRawVideoFrame,
)
from vtk_streaming.vtkStreamingEncode import vtkVideoEncoder
from vtk_streaming.vtkStreamingNvEncode import vtkNvEncoderGL
from vtk_streaming.vtkStreamingOpenGL2 import vtkOpenGLVideoFrame
from vtk_streaming.vtkStreamingVpxEncode import vtkVpxEncoder

from tests import video_test_utils

pytestmark = video_test_utils.requires_rendering()

SIZES = [(320, 240), (480, 360), (160, 120)]
FRAMES_PER_SIZE = 10
THRESHOLD = 0.05


def _make_picture(window, width, height, pixel_format):
    picture = vtkOpenGLVideoFrame()
    picture.SetContext(window)
    picture.SetWidth(width)
    picture.SetHeight(height)
    picture.SetPixelFormat(pixel_format)
    picture.SetSliceOrderType(vtkRawVideoFrame.TopDown)
    picture.ComputeDefaultStrides()
    picture.AllocateDataStore()
    return picture


def _encode_bars_segment(encoder, picture, window, width, height):
    est_size = vtkRawVideoFrame.GetEstimatedSize(width, height, VTKPF_IYUV)
    for shift in range(FRAMES_PER_SIZE):
        frame_bytes = video_test_utils.iyuv_frame_bytes(
            video_test_utils.generate_rgba32_color_bars(width, height, shift)
        )
        assert len(frame_bytes) == est_size  # plane offsets rely on this
        video_test_utils.upload_iyuv_frame(picture, frame_bytes, width, height)
        picture.Render(window)
        encoder.Encode(picture)


def test_vpx_encoder_decoder_resize(tmp_path):
    pytest.importorskip(
        "vtk_streaming.vtkStreamingVpxDecode",
        reason="installed vtk_streaming wheel was built without the VpxDecode module",
    )

    input_window = video_test_utils.make_offscreen_window(*SIZES[0])
    decoded_window = video_test_utils.make_offscreen_window(*SIZES[0])
    decoder, decoded_frame_sizes = video_test_utils.make_decode_render_loop(
        decoded_window
    )

    packet_dims = []

    @calldata_type(VTK_OBJECT)
    def receive_packet(_encoder, _event, packet):
        packet_dims.append((packet.GetDisplayWidth(), packet.GetDisplayHeight()))
        decoder.Decode(packet)

    encoder = vtkVpxEncoder()
    encoder.AddObserver(vtkVideoEncoder.EncodedVideoChunkEvent, receive_packet)
    encoder.SetGraphicsContext(input_window)
    encoder.SetCodec(VTKVC_VP9)
    encoder.SetInputPixelFormat(VTKPF_IYUV)

    for width, height in SIZES:
        input_window.SetSize(width, height)
        input_window.Render()
        decoded_window.SetSize(width, height)
        decoded_window.Render()
        # A fresh frame per size; the first Encode at the new dimensions makes
        # the encoder rebuild its context.
        picture = _make_picture(input_window, width, height, VTKPF_IYUV)
        _encode_bars_segment(encoder, picture, input_window, width, height)

        # Both windows show this size's last frame; compare before moving on.
        scratch = tmp_path / f"{width}x{height}"
        scratch.mkdir()
        result = video_test_utils.regression_compare_windows(
            input_window, decoded_window, THRESHOLD, scratch
        )
        assert result == vtkTesting.PASSED, f"image mismatch at {width}x{height}"
    encoder.Drain()

    expected = [size for size in SIZES for _ in range(FRAMES_PER_SIZE)]
    assert packet_dims == expected
    assert decoded_frame_sizes == expected

    encoder.Shutdown()
    decoder.Shutdown()


def _nal_unit_types(packet):
    """Yield the nal_unit_type of every NAL unit in an Annex B packet.

    Emulation prevention guarantees the 3-byte start code pattern (which is
    also the tail of the 4-byte form) never occurs inside a NAL payload, so a
    plain scan finds exactly the real NAL boundaries.
    """
    index = packet.find(b"\x00\x00\x01")
    while index != -1:
        yield packet[index + 3] & 0x1F
        index = packet.find(b"\x00\x00\x01", index + 3)


@video_test_utils.requires_nvenc()
def test_nv_encoder_resize():
    input_window = video_test_utils.make_offscreen_window(*SIZES[0])

    packets = []
    packet_dims = []

    @calldata_type(VTK_OBJECT)
    def receive_packet(_encoder, _event, packet):
        packets.append(vtk_to_numpy(packet.GetData()).tobytes())
        packet_dims.append((packet.GetDisplayWidth(), packet.GetDisplayHeight()))

    encoder = vtkNvEncoderGL()
    encoder.AddObserver(vtkVideoEncoder.EncodedVideoChunkEvent, receive_packet)
    encoder.SetGraphicsContext(input_window)
    encoder.SetCodec(VTKVC_H264)
    encoder.SetInputPixelFormat(VTKPF_IYUV)

    for width, height in SIZES:
        input_window.SetSize(width, height)
        input_window.Render()
        picture = _make_picture(input_window, width, height, VTKPF_IYUV)
        _encode_bars_segment(encoder, picture, input_window, width, height)
    encoder.Drain()

    assert len(packets) == len(SIZES) * FRAMES_PER_SIZE
    assert all(any(packet) for packet in packets)
    expected = [size for size in SIZES for _ in range(FRAMES_PER_SIZE)]
    assert packet_dims == expected
    # Every resize tears the NVENC session down, so each size must open a new
    # H.264 stream: SPS (7) and PPS (8) parameter sets plus an IDR slice (5).
    for segment in range(len(SIZES)):
        first_packet = packets[segment * FRAMES_PER_SIZE]
        nal_types = set(_nal_unit_types(first_packet))
        assert {7, 8, 5} <= nal_types, (
            f"first packet at {SIZES[segment]} lacks SPS/PPS/IDR; "
            f"NAL types found: {sorted(nal_types)}"
        )

    encoder.Shutdown()
