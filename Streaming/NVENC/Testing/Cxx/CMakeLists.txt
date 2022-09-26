vtk_add_test_cxx(vtkStreamingNVENCCxxTests tests
  TestNvEncodeFunctionLoader.cxx,NO_DATA,NO_VALID,NO_OUTPUT
  TestNvEncoderGLMapResource.cxx,NO_DATA,NO_VALID,NO_OUTPUT
  TestNvEncoderGLPushReceiveIYUV.cxx,NO_VALID
  TestNvEncoderGLPushReceiveNV12.cxx,NO_VALID
  TestNvEncoderGLPushReceiveRGBA32.cxx,NO_VALID
  )

vtk_test_cxx_executable(vtkStreamingNVENCCxxTests tests)
