vtk_add_test_cxx(vtkStreamingOpenGL2CxxTests tests
  TestOpenGLVideoFrameXferNoExternalStrides.cxx,NO_DATA,NO_VALID,NO_OUTPUT
  TestOpenGLVideoFrameXferWithExternalStrides.cxx,NO_DATA,NO_VALID,NO_OUTPUT
  TestRGBA32SliceOrderBottomUp.cxx,NO_DATA,NO_VALID,NO_OUTPUT
  TestRGBA32SliceOrderTopDown.cxx,NO_DATA,NO_VALID,NO_OUTPUT
  TestIYUVSliceOrderTopDown.cxx,NO_VALID,NO_OUTPUT
  TestNV12SliceOrderTopDown.cxx,NO_VALID,NO_OUTPUT
  TestNV12BottomUpCapture.cxx,NO_VALID,NO_OUTPUT
  TestNV12TopDownCapture.cxx,NO_VALID,NO_OUTPUT
  TestIYUVBottomUpCapture.cxx,NO_VALID,NO_OUTPUT
  TestIYUVTopDownCapture.cxx,NO_VALID,NO_OUTPUT
  )

vtk_test_cxx_executable(vtkStreamingOpenGL2CxxTests tests)
