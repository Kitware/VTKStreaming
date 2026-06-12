"""Shared helpers for the VPX encoder/decoder round-trip tests.

These tests render with OpenGL and exercise the real libvpx codec, so they
only run where a working OpenGL context can be created; guard every test
module that uses them with ``requires_rendering()`` so headless CI skips them
(see "Testing constraints" in CLAUDE.md).
"""

import functools
import subprocess
import sys

import numpy as np
import pytest

from vtkmodules.util.numpy_support import numpy_to_vtk

# Bar colors from vtkStreamingTestUtility::GenerateRGBA32ColorBars:
# white, yellow, cyan, green, magenta, red, blue, black.
_BAR_COLORS = np.array(
    [
        [255, 255, 255, 255],
        [255, 255, 0, 255],
        [0, 255, 255, 255],
        [0, 255, 0, 255],
        [255, 0, 255, 255],
        [255, 0, 0, 255],
        [0, 0, 255, 255],
        [0, 0, 0, 255],
    ],
    dtype=np.uint8,
)


def generate_rgba32_color_bars(width, height, shift=0):
    """numpy port of vtkStreamingTestUtility::GenerateRGBA32ColorBars.

    Returns a (height, width, 4) uint8 array of vertical color bars; the top
    and bottom halves scroll in opposite directions as ``shift`` increases.
    """
    ndivs = (width + 7) >> 3
    column_bar = np.arange(width) // ndivs
    top_bar = (column_bar + (shift % 8)) % 8
    bottom_bar = (7 - column_bar + (shift % 8)) % 8
    frame = np.empty((height, width, 4), dtype=np.uint8)
    frame[: height >> 1] = _BAR_COLORS[top_bar]
    frame[height >> 1 :] = _BAR_COLORS[bottom_bar]
    return frame


def rgba_to_yuv420_planes(rgba):
    """Convert an RGBA frame to limited-range BT.709 4:2:0 Y, U, V planes.

    The matrix is the inverse of the YCbCr-to-RGB matrix hard-coded in
    Streaming/OpenGL2/glsl/vtkIYUVRenderFS.glsl so a render of the generated
    planes reproduces the original colors.
    """
    rgb = rgba[..., :3].astype(np.float64) / 255.0
    r, g, b = rgb[..., 0], rgb[..., 1], rgb[..., 2]
    y = 0.2126 * r + 0.7152 * g + 0.0722 * b
    cb = (b - y) / 1.8556
    cr = (r - y) / 1.5748

    def subsample(plane):
        rows, cols = plane.shape
        return plane.reshape(rows // 2, 2, cols // 2, 2).mean(axis=(1, 3))

    def quantize(plane, scale, offset):
        return np.clip(np.round(offset + scale * plane), 0, 255).astype(np.uint8)

    return (
        quantize(y, 219.0, 16.0),
        quantize(subsample(cb), 224.0, 128.0),
        quantize(subsample(cr), 224.0, 128.0),
    )


def iyuv_frame_bytes(rgba):
    """Pack an RGBA frame as one packed I420 (a.k.a. IYUV) frame."""
    y, u, v = rgba_to_yuv420_planes(rgba)
    return y.tobytes() + u.tobytes() + v.tobytes()


def nv12_frame_bytes(rgba):
    """Pack an RGBA frame as one packed NV12 frame (Y plane + interleaved UV)."""
    y, u, v = rgba_to_yuv420_planes(rgba)
    uv = np.empty((u.shape[0], u.shape[1] * 2), dtype=np.uint8)
    uv[:, 0::2] = u
    uv[:, 1::2] = v
    return y.tobytes() + uv.tobytes()


def rgba32_frame_bytes(rgba):
    """Pack an RGBA frame as one packed RGBA32 frame."""
    return rgba.tobytes()


def as_vtk_uchar_array(buf):
    """Copy a bytes-like object into a vtkUnsignedCharArray."""
    return numpy_to_vtk(np.frombuffer(buf, dtype=np.uint8), deep=True)


def upload_iyuv_frame(picture, frame_bytes, width, height):
    """Copy one packed I420 frame into a vtkOpenGLVideoFrame, plane by plane."""
    luma_size = width * height
    chroma_rowsize = width >> 1
    chroma_numrows = (height + 1) >> 1
    chroma_size = chroma_rowsize * chroma_numrows
    picture.CopyPlanarData(
        as_vtk_uchar_array(frame_bytes[:luma_size]), width, height, 0
    )
    picture.CopyPlanarData(
        as_vtk_uchar_array(frame_bytes[luma_size : luma_size + chroma_size]),
        chroma_rowsize,
        chroma_numrows,
        1,
    )
    picture.CopyPlanarData(
        as_vtk_uchar_array(frame_bytes[luma_size + chroma_size :]),
        chroma_rowsize,
        chroma_numrows,
        2,
    )


def upload_nv12_frame(picture, frame_bytes, width, height):
    """Copy one packed NV12 frame into a vtkOpenGLVideoFrame, plane by plane."""
    luma_size = width * height
    picture.CopyPlanarData(
        as_vtk_uchar_array(frame_bytes[:luma_size]), width, height, 0
    )
    picture.CopyPlanarData(
        as_vtk_uchar_array(frame_bytes[luma_size:]), width, (height + 1) >> 1, 1
    )


def upload_rgba32_frame(picture, frame_bytes, width, height):
    """Copy one packed RGBA32 frame into a vtkOpenGLVideoFrame."""
    picture.CopyData(as_vtk_uchar_array(frame_bytes), width * 4, height)


_RENDER_PROBE = """
import vtkmodules.vtkRenderingOpenGL2  # noqa: F401 (register the OpenGL factory)
from vtkmodules.vtkRenderingCore import vtkRenderWindow
win = vtkRenderWindow()
win.SetOffScreenRendering(True)
win.SetSize(32, 32)
win.Render()
"""


@functools.lru_cache(maxsize=1)
def rendering_available():
    """Whether an OpenGL render window can be created on this machine.

    Probed in a subprocess because a failed context creation can abort the
    whole process rather than raise.
    """
    try:
        probe = subprocess.run(
            [sys.executable, "-c", _RENDER_PROBE], capture_output=True, timeout=120
        )
    except (OSError, subprocess.TimeoutExpired):
        return False
    return probe.returncode == 0


def requires_rendering():
    return pytest.mark.skipif(
        not rendering_available(),
        reason="cannot create an OpenGL render window (headless environment)",
    )


# vtkNvEncoderGL::CheckAvailability() calls cuGLGetDevices, which needs a
# current OpenGL context on the CUDA device; without one it reports NVENC as
# unavailable even on NVIDIA hardware.
_NVENC_PROBE = """
import vtkmodules.vtkRenderingOpenGL2  # noqa: F401 (register the OpenGL factory)
from vtkmodules.vtkRenderingCore import vtkRenderWindow
win = vtkRenderWindow()
win.SetOffScreenRendering(True)
win.SetSize(32, 32)
win.Render()
from vtk_streaming.vtkStreamingNvEncode import vtkNvEncoderGL
raise SystemExit(0 if vtkNvEncoderGL.CheckAvailability() else 1)
"""


@functools.lru_cache(maxsize=1)
def nvenc_available():
    """Whether NVENC encoding is available, probed under a live GL context."""
    try:
        probe = subprocess.run(
            [sys.executable, "-c", _NVENC_PROBE], capture_output=True, timeout=120
        )
    except (OSError, subprocess.TimeoutExpired):
        return False
    return probe.returncode == 0


def requires_nvenc():
    return pytest.mark.skipif(
        not nvenc_available(),
        reason="NVENC is not available (no NVIDIA driver, or OpenGL is not on "
        "the NVIDIA GPU)",
    )


def make_offscreen_window(width, height):
    """Create and initialize an offscreen render window.

    Offscreen keeps the framebuffer exactly width x height (no window-manager
    resizing) so the regression image comparison is stable.
    """
    from vtkmodules.vtkRenderingCore import vtkRenderWindow

    window = vtkRenderWindow()
    window.SetOffScreenRendering(True)
    window.SetSize(width, height)
    window.Render()
    return window


def make_decode_render_loop(window):
    """Create a vtkVpxDecoder that renders every decoded frame into ``window``.

    Returns (decoder, decoded_frame_sizes) where decoded_frame_sizes collects
    a (width, height) tuple per decoded frame. Feed the decoder by calling
    ``decoder.Decode(packet)`` with packets observed from an encoder.

    Do not call ``decoder.Drain()``: vtkVpxDecoder::SendEOS() forwards a null
    packet that DecodeInternal dereferences. ``Shutdown()`` is safe.
    """
    from vtkmodules.util.misc import calldata_type
    from vtkmodules.util.vtkConstants import VTK_OBJECT

    from vtk_streaming.vtkStreamingDecode import vtkVideoDecoder
    from vtk_streaming.vtkStreamingVpxDecode import vtkVpxDecoder

    decoder = vtkVpxDecoder()
    decoder.SetGraphicsContext(window)
    decoded_frame_sizes = []

    @calldata_type(VTK_OBJECT)
    def render_decoded_frame(_decoder, _event, frame):
        frame.Render(window)
        decoded_frame_sizes.append((frame.GetWidth(), frame.GetHeight()))

    decoder.AddObserver(vtkVideoDecoder.DecodedVideoFrameEvent, render_decoded_frame)
    return decoder, decoded_frame_sizes


def regression_compare_windows(baseline_window, test_window, threshold, scratch_dir):
    """Compare the current contents of two render windows with vtkTesting.

    The baseline window's framebuffer is written out as the valid image, then
    ``vtkTesting.RegressionTest`` grades the test window's framebuffer against
    it. Returns vtkTesting.PASSED when the images match within ``threshold``;
    difference images land in ``scratch_dir`` on failure.
    """
    import os

    from vtkmodules.vtkIOImage import vtkPNGWriter
    from vtkmodules.vtkRenderingCore import vtkWindowToImageFilter
    from vtkmodules.vtkTestingRendering import vtkTesting

    # Pin the comparison method: the SSIM-based TIGHT_VALID/LOOSE_VALID
    # methods cannot tell codec blur from a genuinely different image here
    # (measured 0.113 for a VP9 round-trip vs 0.133 for a different camera
    # angle), while the legacy metric separates them by orders of magnitude
    # (0 vs ~1000). Exported VTK_TESTING_IMAGE_COMPARE_METHOD still wins.
    os.environ.setdefault("VTK_TESTING_IMAGE_COMPARE_METHOD", "LEGACY_VALID")

    baseline_png = str(scratch_dir / "baseline.png")
    grab_baseline = vtkWindowToImageFilter()
    grab_baseline.SetInput(baseline_window)
    grab_baseline.ShouldRerenderOff()
    writer = vtkPNGWriter()
    writer.SetFileName(baseline_png)
    writer.SetInputConnection(grab_baseline.GetOutputPort())
    writer.Write()

    grab_test = vtkWindowToImageFilter()
    grab_test.SetInput(test_window)
    grab_test.ShouldRerenderOff()

    testing = vtkTesting()
    testing.AddArgument("-T")
    testing.AddArgument(str(scratch_dir))
    testing.AddArgument("-V")
    testing.AddArgument(baseline_png)
    return testing.RegressionTest(grab_test, threshold)
