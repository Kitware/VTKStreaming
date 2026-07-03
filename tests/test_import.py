def test_import():
    """
    This is a simple test that import the package and checks that it is found.
    We can't really test more as runners do not have a NVidia GPU.
    """
    from vtk_streaming.vtkStreamingCore import vtkCompressedVideoPacket  # noqa
    from vtk_streaming.vtkStreamingOpenGL2 import vtkOpenGLVideoFrame  # noqa
    from vtk_streaming.vtkStreamingNvEncode import vtkNvEncoderGL  # noqa
