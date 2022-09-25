vtk_add_test_cxx(vtkStreamingNVENCCxxTests tests
  TestNvEncodeFunctionLoader.cxx,NO_DATA,NO_VALID,NO_OUTPUT
  TestNvEncoderMapResource.cxx,NO_DATA,NO_VALID,NO_OUTPUT
  TestNvEncoderPushReceiveIYUV.cxx,NO_VALID,NO_OUTPUT
  TestNvEncoderPushReceiveNV12.cxx,NO_VALID,NO_OUTPUT
  TestNvEncoderPushReceiveRGBA32.cxx,NO_DATA,NO_VALID,NO_OUTPUT
  )

vtk_test_cxx_executable(vtkStreamingNVENCCxxTests tests)
