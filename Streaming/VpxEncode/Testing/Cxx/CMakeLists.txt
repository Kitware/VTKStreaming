vtk_add_test_cxx(vtkStreamingVpxEncodeCxxTests tests
  TestVpxEncoderPushReceiveIYUV.cxx.cxx,NO_VALID
  TestVpxEncoderPushReceiveNV12.cxx.cxx,NO_VALID
  TestVpxEncoderPushReceiveDisplayIYUV.cxx,NO_VALID
  TestVpxEncoderPushReceiveDisplayNV12.cxx,NO_VALID
  )

vtk_test_cxx_executable(vtkStreamingVpxEncodeCxxTests tests)
