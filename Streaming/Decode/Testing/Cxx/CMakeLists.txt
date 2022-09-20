vtk_add_test_cxx(vtkStreamingDecodeCxxTests tests
    TestDecoderDelegateUsage.cxx,NO_DATA,NO_VALID,NO_OUTPUT
    TestDecoderAsyncDelegate.cxx,NO_DATA,NO_VALID,NO_OUTPUT
  )

vtk_test_cxx_executable(vtkStreamingDecodeCxxTests tests)
