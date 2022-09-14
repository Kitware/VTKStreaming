vtk_add_test_cxx(vtkStreamingCoreCxxTests tests
  TestCodedVideoPacketMemory.cxx,NO_DATA,NO_VALID,NO_OUTPUT
  TestVideoFrameLumaChroma.cxx,NO_DATA,NO_VALID,NO_OUTPUT
  TestCPUVideoFrameXferNoExternalStrides.cxx,NO_DATA,NO_VALID,NO_OUTPUT
  TestCPUVideoFrameXferWithExternalStrides.cxx,NO_DATA,NO_VALID,NO_OUTPUT
  )

vtk_test_cxx_executable(vtkStreamingCoreCxxTests tests)
