vtk_add_test_cxx(vtkStreamingEncodeCxxTests tests
  TestEncoderDelegateUsage.cxx,NO_DATA,NO_VALID,NO_OUTPUT
  )

vtk_test_cxx_executable(vtkStreamingEncodeCxxTests tests)
