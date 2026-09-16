// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-License-Identifier: Apache-2.0
/**
 * @class   vtkVulkanEncoderInternals
 * @brief   Vulkan Video H.264 session state: images, DPB, bitstream buffer, encode submit.
 */

#ifndef vtkVulkanEncoderInternals_h
#define vtkVulkanEncoderInternals_h

#include "vtkStreamingVulkanEncodeModule.h"

#include "vtkVulkanVideoDevice.h"

#include <chrono>
#include <cstdint>
#include <initializer_list>
#include <string>
#include <vector>

class VTKSTREAMINGVULKANENCODE_NO_EXPORT vtkVulkanEncoderInternals
{
public:
  explicit vtkVulkanEncoderInternals(vtkVulkanVideoDevice& device);
  ~vtkVulkanEncoderInternals();
  vtkVulkanEncoderInternals(const vtkVulkanEncoderInternals&) = delete;
  void operator=(const vtkVulkanEncoderInternals&) = delete;

  enum class RateControlMode
  {
    ConstantBitrate,
    VariableBitrate,
    ConstantQp
  };

  struct Config
  {
    int Width = 0;
    int Height = 0;
    int GopSize = 10;
    uint32_t FrameRateNumerator = 30;
    uint32_t FrameRateDenominator = 1;
    RateControlMode RateControl = RateControlMode::ConstantBitrate;
    uint64_t AverageBitrate = 1000000;
    uint64_t MaxBitrate = 1000000;
    int Qp = 26;
    int MinQp = 0;
    int MaxQp = 51;
    bool LowDelay = true;
    // The input image holds a bottom-up picture (an OpenGL framebuffer); flip it while converting.
    bool FlipInput = false;
  };

  struct EncodedFrame
  {
    std::vector<unsigned char> Bitstream;
    bool IsKeyFrame = false;
    int64_t PresentationTS = 0;
  };

  bool Setup(const Config& config);
  void TearDown();
  bool IsReady() const { return this->Session != VK_NULL_HANDLE; }

  ///@{
  /**
   * Encodes the RGBA8 frame an external GL writer (see vtkVulkanGLInterop) blitted into the
   * input image, synchronized through `ReadySemaphore`/`FreeSemaphore`: the compute queue
   * waits on `ReadySemaphore`, converts the input image to NV12 with a compute shader and
   * signals `FreeSemaphore` (the input image may be written into again) plus
   * `ConvertDoneSemaphore`, which the encode queue waits on before encoding. Video encode
   * queue families rarely support compute, hence the two queues.
   *
   * Where the driver allows storage-image writes to a video encode source image, the shader
   * writes the NV12 planes in place. Otherwise (Mesa 25.0 and NVIDIA 615 both refuse) it
   * writes two intermediate R8/RG8 images that are then copied into the NV12 image on the
   * same queue.
   */
  bool EncodeFrame(bool forceKeyFrame, EncodedFrame& out);
  // The input image is an ordinary single-plane RGBA8 image of the aligned coded size backed
  // by a dedicated, opaque-fd exportable allocation; GL imports it as a GL_RGBA8 texture and
  // blits the window's colour buffer into it. Only the top-left Config::Width x Height
  // region needs to be written.
  int ExportInputMemoryFd() const;
  VkDeviceSize GetInputMemorySize() const;
  int ExportReadySemaphoreFd() const;
  int ExportFreeSemaphoreFd() const;
  uint32_t GetAlignedWidth() const { return this->AlignedWidth; }
  uint32_t GetAlignedHeight() const { return this->AlignedHeight; }
  ///@}

  /**
   * WebCodecs/RFC-6381 codec string ("avc1.PPCCLL") parsed from the emitted SPS.
   * Empty until Setup() succeeds.
   */
  const std::string& GetCodecName() const { return this->CodecName; }

  std::chrono::high_resolution_clock::duration dtEncode{};

private:
  struct Buffer
  {
    VkBuffer Handle = VK_NULL_HANDLE;
    VkDeviceMemory Memory = VK_NULL_HANDLE;
    VkDeviceSize Size = 0;
    void* Mapped = nullptr;
  };
  struct Image
  {
    VkImage Handle = VK_NULL_HANDLE;
    VkDeviceMemory Memory = VK_NULL_HANDLE;
    VkDeviceSize MemorySize = 0;
    VkImageView View = VK_NULL_HANDLE;
    uint32_t Layers = 1;
  };

  bool QueryCapabilities();
  bool QueryFormats();
  bool CreateSession();
  bool CreateSessionParameters();
  bool EmitParameterSets();
  bool CreateNV12SourceImage();
  bool CreateInputImage();
  bool CreatePlaneImages();
  bool CreateConvertPipeline();
  void DestroyConvertPipeline();
  bool CreateDpbImage();
  bool CreateBuffers();
  bool CreateQueryPool();
  bool CreateCommandResources();
  bool TransitionDpbToEncodeLayout();

  // Exportable images are plain (non-video) images backed by dedicated, opaque-fd exportable
  // memory; everything else is created against the H.264 profile. `sharedWithComputeQueue`
  // makes the image usable from both the compute and the encode queue families.
  bool CreateImage(VkImageUsageFlags usage, VkFormat format, VkExtent2D extent, uint32_t layers,
    Image& image, bool exportable = false, bool sharedWithComputeQueue = false,
    VkImageCreateFlags flags = 0);
  bool CreateImageView(
    VkImage image, VkImageAspectFlags aspect, VkFormat format, uint32_t layers, VkImageView& view);
  bool CreateBuffer(VkBufferUsageFlags usage, VkDeviceSize size, bool hostVisible, Buffer& buffer);
  void DestroyImage(Image& image);
  void DestroyBuffer(Buffer& buffer);
  bool AllocateMemory(const VkMemoryRequirements& reqs, VkMemoryPropertyFlags required,
    VkMemoryPropertyFlags preferred, VkDeviceMemory& memory);
  bool AllocateExportableMemory(
    const VkMemoryRequirements& reqs, VkImage dedicatedImage, VkDeviceMemory& memory);
  bool CreateSemaphore(VkSemaphore& semaphore, bool exportable);
  int ExportSemaphoreFd(VkSemaphore semaphore) const;
  bool InitializeInteropImageLayout();
  // `blockUntilDone` waits on the fence for the work to finish; command buffers may only be
  // reset after that.
  bool Submit(VkQueue queue, VkCommandBuffer cmd, VkSemaphore wait,
    std::initializer_list<VkSemaphore> signals, bool blockUntilDone);

  void RecordConvert(VkCommandBuffer cmd);
  void RecordEncode(VkCommandBuffer cmd, bool keyFrame);
  bool ReadBackBitstream(EncodedFrame& out);
  bool FinishEncodedFrame(bool keyFrame, EncodedFrame& out);
  // Mesa < 26.0 (ANV) workaround, see StripSliceHeaderPadding in the .cxx.
  void FixSliceHeaderPadding(std::vector<unsigned char>& bitstream) const;

  vtkVulkanVideoDevice& Device;
  Config Settings;
  uint32_t AlignedWidth = 0;
  uint32_t AlignedHeight = 0;

  // Profile chain; addresses are baked into other create infos, so keep them stable.
  VkVideoEncodeH264ProfileInfoKHR H264Profile = {};
  VkVideoProfileInfoKHR Profile = {};
  VkVideoProfileListInfoKHR ProfileList = {};
  VkVideoEncodeH264CapabilitiesKHR H264Capabilities = {};
  VkVideoEncodeCapabilitiesKHR EncodeCapabilities = {};
  VkVideoCapabilitiesKHR Capabilities = {};
  VkFormat SourceFormat = VK_FORMAT_UNDEFINED;
  VkFormat DpbFormat = VK_FORMAT_UNDEFINED;
  // True when the conversion shader can store straight into the NV12 source image's planes
  // (needs VK_IMAGE_USAGE_STORAGE_BIT and per-plane views on a video encode source image).
  bool DirectConvert = false;
  VkImageCreateFlags SourceImageCreateFlags = 0;

  VkVideoSessionKHR Session = VK_NULL_HANDLE;
  std::vector<VkDeviceMemory> SessionMemory;
  VkVideoSessionParametersKHR SessionParameters = VK_NULL_HANDLE;
  StdVideoH264SequenceParameterSet Sps = {};
  StdVideoH264PictureParameterSet Pps = {};
  std::vector<unsigned char> ParameterSetBytes;
  std::string CodecName;

  Image SourceImage;
  VkImageView SourcePlaneViews[2] = {}; // R8 / RG8 storage views of SourceImage (DirectConvert).
  Image InputImage;                     // GL-written RGBA8, exportable.
  Image PlaneImages[2];                 // Shader-written luma (R8) and chroma (RG8) when
                                        // !DirectConvert, copied into SourceImage.
  Image DpbImage;
  Buffer BitstreamBuffer;
  VkQueryPool QueryPool = VK_NULL_HANDLE;

  // RGBA -> NV12 compute pass (vtkVulkanRGBAToNV12CS.h).
  VkShaderModule ConvertShader = VK_NULL_HANDLE;
  VkDescriptorSetLayout ConvertSetLayout = VK_NULL_HANDLE;
  VkPipelineLayout ConvertPipelineLayout = VK_NULL_HANDLE;
  VkPipeline ConvertPipeline = VK_NULL_HANDLE;
  VkDescriptorPool ConvertDescriptorPool = VK_NULL_HANDLE;
  VkDescriptorSet ConvertDescriptorSet = VK_NULL_HANDLE;

  VkCommandPool EncodeCommandPool = VK_NULL_HANDLE;
  VkCommandBuffer EncodeCommandBuffer = VK_NULL_HANDLE;
  VkCommandPool ComputeCommandPool = VK_NULL_HANDLE;
  VkCommandBuffer ComputeCommandBuffer = VK_NULL_HANDLE;
  VkFence Fence = VK_NULL_HANDLE;

  // OpenGL/Vulkan interop: exportable input image, written directly by GL, no CPU upload.
  VkSemaphore ReadySemaphore = VK_NULL_HANDLE; // GL signals, Vulkan waits: frame ready.
  VkSemaphore FreeSemaphore = VK_NULL_HANDLE;  // Vulkan signals, GL waits: input free to reuse.
  VkSemaphore ConvertDoneSemaphore = VK_NULL_HANDLE; // compute queue -> encode queue.

  // Rate control state, re-chained into every vkCmdBeginVideoCodingKHR after it is set.
  VkVideoEncodeH264RateControlLayerInfoKHR H264RateControlLayer = {};
  VkVideoEncodeRateControlLayerInfoKHR RateControlLayer = {};
  VkVideoEncodeH264RateControlInfoKHR H264RateControl = {};
  VkVideoEncodeRateControlInfoKHR RateControl = {};
  bool RateControlApplied = false;
  bool NeedsSliceHeaderPaddingFix = false;

  // GOP / DPB bookkeeping (IPPP..., one reference, two ping-pong slots).
  uint32_t FrameCount = 0;
  uint32_t FrameIndexInGop = 0;
  uint32_t FrameNum = 0;
  uint16_t IdrPicId = 0;
  int32_t CurrentSlot = 0;
  bool SlotActive[2] = {};
  bool HaveReference = false;
  StdVideoEncodeH264ReferenceInfo ReferenceInfo = {};
};

#endif // vtkVulkanEncoderInternals_h
// VTK-HeaderTest-Exclude: vtkVulkanEncoderInternals.h
