/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkNvEncoderInternals.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkCompressedVideoPacket.h"
#include "vtkNvEncodeLoader.h"
#include "vtkPixelFormatTypes.h"
#include "vtkRawVideoFrame.h"
#include "vtkVideoProcessingStatusTypes.h"

#include "nvEncodeAPI.h"

#include <chrono>

class vtkVideoEncoder;

class vtkNvEncoderInternals
{
public:
  vtkNvEncoderInternals();
  ~vtkNvEncoderInternals();

  // input frames.
  std::vector<vtkSmartPointer<vtkRawVideoFrame>> NvEncInputFrames;
  std::vector<void*> NvEncInputResources;

  // timing for encode and upload ops.
  std::chrono::high_resolution_clock::time_point::duration dtEncode, dtUpload;

  /**
   * Loads the shared library nvEncodeAPI.dll on windows, libnvidia-encode.so on linux.
   * Loads key function pointers from the shared library to create an NvEnc API instance
   * and open an encoder session.
   */
  bool OpenEncodeSession(NV_ENC_DEVICE_TYPE deviceType, void* deviceHandle, int width, int height,
    NV_ENC_BUFFER_FORMAT pixFmt);

  /**
   * Populate the parameters based on type of codec, preset and tuning information.
   * You can use this convenient function to generate a basic set of parameters that can
   * be used in ::InitializeEncodeCtx().
   * If needed, you can alos override the parameters with app-specific settings before
   * ::InitializeEncodeCtx()
   */
  bool CreateDefaultEncoderInitializeParams(
    NV_ENC_INITIALIZE_PARAMS* params, GUID codecGuid, GUID presetGuid, NV_ENC_TUNING_INFO tuneInfo);

  /**
   * Initializes encoder session and output buffers.
   */
  bool InitializeEncodeCtx(const NV_ENC_INITIALIZE_PARAMS* params);

  /**
   * Returns true only if an encoder session is open and initialized for encoding.
   */
  bool IsInitialized() { return this->NvEncSession != nullptr && this->NvEncInitialized; }

  /**
   * Registers cuda/d3d/opengl input buffers with NvEncodeAPI.
   */
  bool RegisterInputResources(const std::vector<void*>& inputResources,
    std::vector<vtkSmartPointer<vtkRawVideoFrame>>& inputFrames,
    NV_ENC_INPUT_RESOURCE_TYPE resourceType, NV_ENC_BUFFER_FORMAT bufferFormat);

  /**
   * Registers cuda/d3d/opengl input or output buffers with NvEncodeAPI.
   */
  NV_ENC_REGISTERED_PTR RegisterResource(void* resource, NV_ENC_INPUT_RESOURCE_TYPE resourceType,
    int width, int height, int pitch, NV_ENC_BUFFER_FORMAT bufferFormat,
    NV_ENC_BUFFER_USAGE bufferUsage = NV_ENC_INPUT_IMAGE,
    NV_ENC_FENCE_POINT_D3D12* inputFencePoint = nullptr,
    NV_ENC_FENCE_POINT_D3D12* outputFencePoint = nullptr);

  /**
   * Submits encode commands to the NVENC chip on the gpu.
   */
  NVENCSTATUS Send(bool keyFrame = false);
  bool SendEOS();

  /**
   * Get the coded packets from the NVENC chip on the gpu.
   */
  bool Receive(
    std::vector<vtkSmartPointer<vtkCompressedVideoPacket>>& packets, bool outputDelay = false);

  /**
   * Submits encode commands to the NVENC chip on the gpu and read back
   * the coded bitstream.
   */
  NVENCSTATUS Encode(std::vector<vtkSmartPointer<vtkCompressedVideoPacket>>& packets,
    bool keyFrame = false, bool outputDelay = false);

  /**
   * Flushes the encoder queue.
   * The encoder may keep references to frames internally for B-frames or lookahead.
   * App must call Flush() to get all the queued encoded frames from the encoder
   * before shutting down the encoder session.
   */
  bool EndEncode(std::vector<vtkSmartPointer<vtkCompressedVideoPacket>>& packets);
  void Flush();

  /**
   * Query hardware encoder capabilities.
   */
  int GetCapability(GUID guidCodec, NV_ENC_CAPS caps);

  /**
   * Get the current initialization parameters.
   */
  void GetInitializeParams(NV_ENC_INITIALIZE_PARAMS* params);

  /**
   * Call this function to get a pointer to the next available input buffer.
   * After you have it, copy uncompressed data to the input buffer and then call
   * either Send() or Encode().
   */
  vtkSmartPointer<vtkRawVideoFrame> GetNextInputFrame() const;

  /**
   *  Get the current device on which encoder is running.
   */
  void* GetDevice() const { return this->NvEncDeviceHandle; }

  /**
   * Get the current device type which encoder is running.
   */
  NV_ENC_DEVICE_TYPE GetDeviceType() const { return this->NvEncDeviceType; }

  /**
   * Get the current encode width.
   */
  int GetEncodeWidth() const { return this->Width; }

  /**
   * Get the current encode height.
   */
  int GetEncodeHeight() const { return this->Height; }

  /**
   * Get the current frame size based on pixel format.
   */
  int GetFrameSize() const;

  /**
   * Returns the number of allocated buffers.
   */
  uint32_t GetEncoderBufferCount() const { return this->NvEncBufferCount; }

  /**
   * Unregister previously registered resources.
   */
  void UnregisterInputResources();

  /**
   * Releases bitstream buffers and shuts down the encoder session.
   * It does not release input buffers as those can be either cuda/gl/dx
   * You should preferably release the cuda/gl/dx resources prior to calling Shutdown.
   */
  bool Shutdown();

  ///@{
  /**
   * Utilities
   */
  static VTKVideoProcessingStatusType ParseNvEncodeAPIStatus(NVENCSTATUS statusCode);
  static std::string FullParamToString(const NV_ENC_INITIALIZE_PARAMS* params);
  static VTKPixelFormatType ParsePixelFormat(NV_ENC_BUFFER_FORMAT nvEncBufFmt);
  static NV_ENC_BUFFER_FORMAT ParsePixelFormat(VTKPixelFormatType nvEncBufFmt);
  static bool TweakFromEncoderObject(
    NV_ENC_INITIALIZE_PARAMS* params, vtkVideoEncoder* encoderObject);
  ///@}

private:
  vtkNvEncoderInternals(const vtkNvEncoderInternals&) = delete;
  void operator=(const vtkNvEncoderInternals&) = delete;

  std::chrono::high_resolution_clock::time_point t1, t2;

  //------------------------------------------------------------------------------
  // 1. Shared library handles and key function pointers.
  //------------------------------------------------------------------------------
  vtkNvEncodeLoader NvEncodeLoader;

  //------------------------------------------------------------------------------
  // 2. Handles to the NvEnc session, device and encoder parameters.
  //------------------------------------------------------------------------------
  // a list of nvenc functions. treat this as an instance of the nvenc API.
  NV_ENCODE_API_FUNCTION_LIST NvEncInstance;
  // hardware encoder session instance.
  void* NvEncSession = nullptr;
  // device handle.
  void* NvEncDeviceHandle = nullptr;
  // track encoder initialization.
  bool NvEncInitialized = false;
  // device type - cuda, gl, dx ..
  NV_ENC_DEVICE_TYPE NvEncDeviceType;
  // pixel format - nv12, yuv444, ...
  NV_ENC_BUFFER_FORMAT NvEncPixelFormat = NV_ENC_BUFFER_FORMAT::NV_ENC_BUFFER_FORMAT_NV12;
  // the parameters used to initialize the encoder after an encode session is opened.
  NV_ENC_INITIALIZE_PARAMS NvEncInitializeParams = {};
  // the encoder configuration.
  NV_ENC_CONFIG NvEncConfig = {};

  //------------------------------------------------------------------------------
  // 3. Resource tracking for encoder inputs and outputs.
  //------------------------------------------------------------------------------
  // mapped input buffers.
  std::vector<NV_ENC_INPUT_PTR> NvEncMappedInputBuffers;
  // mapped output buffers.
  std::vector<NV_ENC_OUTPUT_PTR> NvBitstreamBuffers;
  // resources registered with the encoder.
  std::vector<NV_ENC_REGISTERED_PTR> NvEncRegisteredResources;
  // number of encoder buffers. the encoder may use more than 1 buffer for B-frames or lookahead
  std::size_t NvEncBufferCount = 1;
  // encoder input counter.
  std::size_t NvEncSendCounter = 0;
  // encoder output counter.
  std::size_t NvEncRecvCounter = 0;
  // for low-latency applications, set this to 0 - the default.
  std::size_t NvEncExtraOutputDelay = 0;
  // number of frames that the output is delayed by. > 1 for B-frames or lookahead.
  std::size_t NvEncOutputDelay = 0;

  //------------------------------------------------------------------------------
  // 4. Encoder dimensions
  //------------------------------------------------------------------------------
  // dims
  int Width = 0;
  int Height = 0;
  int MaxWidth = 0;
  int MaxHeight = 0;

  /**
   * Returns true only if the driver supports our nvEncodeAPI.h version
   * and the nvenc api instance is created successfully.
   */
  bool LoadNvEncodeAPI();

  /**
   * Maps frame to NvEncodeAPI.
   */
  bool MapInputBufferToDevice(std::size_t bfrIdx);
  bool InitializeBitstreamBuffers();
  bool ReleaseBitstreamBuffers();

  bool IsZeroDelay() { return this->NvEncOutputDelay == 0; }
};
// VTK-HeaderTest-Exclude: vtkNvEncoderInternals.h
