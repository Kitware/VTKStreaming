// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-License-Identifier: Apache-2.0

#include "vtkNvEncoderInternals.h"
#include "nvEncodeAPI.h"
#include "vtkCompressedVideoPacket.h"
#include "vtkDynamicLoader.h"
#include "vtkLogger.h"
#include "vtkOpenGLVideoFrame.h"
#include "vtkRawVideoFrame.h"
#include "vtkSmartPointer.h"
#include "vtkVideoEncoder.h"
#include "vtkVideoProcessingWorkUnitTypes.h"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

#ifndef _WIN32
#include <cstring>
static inline bool operator==(const GUID& guid1, const GUID& guid2)
{
  return !memcmp(&guid1, &guid2, sizeof(GUID));
}

static inline bool operator!=(const GUID& guid1, const GUID& guid2)
{
  return !(guid1 == guid2);
}
#endif

#define VTK_NVENC_API_CHECKED_INVOKE(nvencAPICall)                                                 \
  do                                                                                               \
  {                                                                                                \
    vtkLogF(TRACE, "NVENC %s", #nvencAPICall);                                                     \
    success = true;                                                                                \
    errorCode = nvencAPICall;                                                                      \
    if (errorCode != NV_ENC_SUCCESS && errorCode != NV_ENC_ERR_NEED_MORE_INPUT)                    \
    {                                                                                              \
      success = false;                                                                             \
      vtkLog(ERROR, << #nvencAPICall << " failed. ErrorCode (" << errorCode << ")");               \
    }                                                                                              \
  } while (0)

namespace
{

std::vector<std::string> codecNames = { "h264", "hevc" };
std::vector<GUID> vCodec = std::vector<GUID>{ NV_ENC_CODEC_H264_GUID, NV_ENC_CODEC_HEVC_GUID };

//------------------------------------------------------------------------------
// Build a WebCodecs/RFC-6381 codec string from the sequence parameter set (SPS)
// carried by an Annex-B key frame bitstream. Returns an empty string when no SPS
// is present (e.g. a delta frame) or the codec is unsupported.
std::string BuildCodecString(const GUID& codec, const unsigned char* data, std::size_t size)
{
  const bool isH264 = (codec == NV_ENC_CODEC_H264_GUID);
  const bool isHEVC = (codec == NV_ENC_CODEC_HEVC_GUID);
  if (data == nullptr || (!isH264 && !isHEVC))
  {
    return {};
  }
  // Walk the Annex-B NAL units looking for the SPS. A leading zero of a 4-byte
  // start code is skipped naturally by advancing one byte at a time.
  for (std::size_t i = 0; i + 3 < size; ++i)
  {
    if (data[i] != 0 || data[i + 1] != 0 || data[i + 2] != 1)
    {
      continue;
    }
    const std::size_t nal = i + 3;
    if (isH264)
    {
      const int nalType = data[nal] & 0x1F;
      if (nalType == 7 && nal + 3 < size) // Sequence parameter set.
      {
        // avc1.PPCCLL: profile_idc, constraint_set flags byte, level_idc.
        char buf[16];
        std::snprintf(
          buf, sizeof(buf), "avc1.%02X%02X%02X", data[nal + 1], data[nal + 2], data[nal + 3]);
        return buf;
      }
    }
    else // HEVC
    {
      const int nalType = (data[nal] >> 1) & 0x3F;
      if (nalType == 33 && nal + 14 < size) // Sequence parameter set.
      {
        // Skip the 2-byte NAL header and the byte holding
        // sps_video_parameter_set_id/sps_max_sub_layers_minus1/
        // sps_temporal_id_nesting_flag to reach profile_tier_level().
        const unsigned char* ptl = &data[nal + 3];
        const int profileSpace = (ptl[0] >> 6) & 0x03;
        const int tierFlag = (ptl[0] >> 5) & 0x01;
        const int profileIdc = ptl[0] & 0x1F;
        const std::uint32_t compat = (std::uint32_t(ptl[1]) << 24) | (std::uint32_t(ptl[2]) << 16) |
          (std::uint32_t(ptl[3]) << 8) | std::uint32_t(ptl[4]);
        const int levelIdc = ptl[11];
        std::ostringstream oss;
        oss << "hvc1.";
        if (profileSpace > 0)
        {
          oss << static_cast<char>('A' + profileSpace - 1);
        }
        oss << profileIdc << '.' << std::uppercase << std::hex << compat << std::dec << '.'
            << (tierFlag ? 'H' : 'L') << levelIdc;
        // Six constraint-indicator-flag bytes, trailing zero bytes trimmed.
        int last = 5;
        while (last >= 0 && ptl[5 + last] == 0)
        {
          --last;
        }
        for (int c = 0; c <= last; ++c)
        {
          char cbuf[8];
          std::snprintf(cbuf, sizeof(cbuf), ".%02X", ptl[5 + c]);
          oss << cbuf;
        }
        return oss.str();
      }
    }
  }
  return {};
}

std::vector<std::string> szPresetNames = { "p1", "p2", "p3", "p4", "p5", "p6", "p7" };
std::vector<GUID> vPreset = std::vector<GUID>{
  NV_ENC_PRESET_P1_GUID,
  NV_ENC_PRESET_P2_GUID,
  NV_ENC_PRESET_P3_GUID,
  NV_ENC_PRESET_P4_GUID,
  NV_ENC_PRESET_P5_GUID,
  NV_ENC_PRESET_P6_GUID,
  NV_ENC_PRESET_P7_GUID,
};

std::vector<std::string> szH264ProfileNames = { "baseline", "main", "high", "high444" };
std::vector<GUID> vH264Profile = std::vector<GUID>{
  NV_ENC_H264_PROFILE_BASELINE_GUID,
  NV_ENC_H264_PROFILE_MAIN_GUID,
  NV_ENC_H264_PROFILE_HIGH_GUID,
  NV_ENC_H264_PROFILE_HIGH_444_GUID,
};
std::vector<std::string> szHevcProfileNames = { "main", "main10", "frext" };
std::vector<GUID> vHevcProfile = std::vector<GUID>{
  NV_ENC_HEVC_PROFILE_MAIN_GUID,
  NV_ENC_HEVC_PROFILE_MAIN10_GUID,
  NV_ENC_HEVC_PROFILE_FREXT_GUID,
};
std::vector<std::string> szProfileNames = { "(default)", "auto", "baseline(h264)", "main(h264)",
  "high(h264)", "high444(h264)", "stereo(h264)", "progressiv_high(h264)", "constrained_high(h264)",
  "main(hevc)", "main10(hevc)", "frext(hevc)" };
std::vector<GUID> vProfile = std::vector<GUID>{
  GUID{},
  NV_ENC_CODEC_PROFILE_AUTOSELECT_GUID,
  NV_ENC_H264_PROFILE_BASELINE_GUID,
  NV_ENC_H264_PROFILE_MAIN_GUID,
  NV_ENC_H264_PROFILE_HIGH_GUID,
  NV_ENC_H264_PROFILE_HIGH_444_GUID,
  NV_ENC_H264_PROFILE_STEREO_GUID,
  NV_ENC_H264_PROFILE_PROGRESSIVE_HIGH_GUID,
  NV_ENC_H264_PROFILE_CONSTRAINED_HIGH_GUID,
  NV_ENC_HEVC_PROFILE_MAIN_GUID,
  NV_ENC_HEVC_PROFILE_MAIN10_GUID,
  NV_ENC_HEVC_PROFILE_FREXT_GUID,
};

std::vector<std::string> szTuningInfoNames = { "hq", "lowlatency", "ultralowlatency", "lossless" };
std::vector<NV_ENC_TUNING_INFO> vTuningInfo =
  std::vector<NV_ENC_TUNING_INFO>{ NV_ENC_TUNING_INFO_HIGH_QUALITY, NV_ENC_TUNING_INFO_LOW_LATENCY,
    NV_ENC_TUNING_INFO_ULTRA_LOW_LATENCY, NV_ENC_TUNING_INFO_LOSSLESS };

std::vector<std::string> szRcModeNames = { "constqp", "vbr", "cbr" };
std::vector<NV_ENC_PARAMS_RC_MODE> vRcMode = std::vector<NV_ENC_PARAMS_RC_MODE>{
  NV_ENC_PARAMS_RC_CONSTQP,
  NV_ENC_PARAMS_RC_VBR,
  NV_ENC_PARAMS_RC_CBR,
};

std::vector<std::string> szMultipass = { "disabled", "qres", "fullres" };
std::vector<NV_ENC_MULTI_PASS> vMultiPass = std::vector<NV_ENC_MULTI_PASS>{
  NV_ENC_MULTI_PASS_DISABLED,
  NV_ENC_TWO_PASS_QUARTER_RESOLUTION,
  NV_ENC_TWO_PASS_FULL_RESOLUTION,
};

std::vector<std::string> szQpMapModeNames = { "disabled", "emphasis_level_map", "delta_qp_map",
  "qp_map" };
std::vector<NV_ENC_QP_MAP_MODE> vQpMapMode = std::vector<NV_ENC_QP_MAP_MODE>{
  NV_ENC_QP_MAP_DISABLED,
  NV_ENC_QP_MAP_EMPHASIS,
  NV_ENC_QP_MAP_DELTA,
  NV_ENC_QP_MAP,
};

std::string join(const std::vector<std::string> values)
{
  std::string result;
  for (const auto& value : values)
  {
    result += (", " + value);
  }
  return result;
}

template <typename T>
std::string ConvertValueToString(
  const std::vector<T>& vValue, const std::vector<std::string>& strValueNames, T value)
{
  auto it = std::find(vValue.begin(), vValue.end(), value);
  if (it == vValue.end())
  {
    vtkLog(ERROR, << "Invalid value. Can't convert to one of " << ::join(strValueNames));
    return std::string();
  }
  return strValueNames[std::distance(vValue.begin(), it)];
}
} // end anonymous

//------------------------------------------------------------------------------
vtkNvEncoderInternals::vtkNvEncoderInternals() = default;

//------------------------------------------------------------------------------
vtkNvEncoderInternals::~vtkNvEncoderInternals() = default;

//------------------------------------------------------------------------------
bool vtkNvEncoderInternals::OpenEncodeSession(NV_ENC_DEVICE_TYPE deviceType, void* deviceHandle,
  int width,  // NOLINT(bugprone-easily-swappable-parameters)
  int height, // NOLINT(bugprone-easily-swappable-parameters)
  NV_ENC_BUFFER_FORMAT pixFmt)
{
  vtkLogScopeFunction(TRACE);
  if (!this->NvEncodeLoader.LoadFunctionsTable())
  {
    vtkLog(ERROR, << "Failed to load NvEncode library."
                  << "Please install or upgrade NVIDIA drivers if you have an NVIDIA GPU.");
    return false;
  }
  if (!this->LoadNvEncodeAPI())
  {
    vtkLog(ERROR, << "Failed to load NvEncodeAPI."
                  << "Please install or upgrade NVIDIA drivers if you have an NVIDIA GPU.");
    return false;
  }

  if (!this->NvEncInstance.nvEncOpenEncodeSession)
  {
    vtkLog(ERROR, << "EncodeAPI not found" << NV_ENC_ERR_NO_ENCODE_DEVICE);
    return false;
  }

  NV_ENC_OPEN_ENCODE_SESSION_EX_PARAMS encodeSessionExParams = {
    NV_ENC_OPEN_ENCODE_SESSION_EX_PARAMS_VER
  };

  this->NvEncDeviceHandle = deviceHandle;
  this->NvEncDeviceType = deviceType;

  this->Width = width;
  this->Height = height;
  this->NvEncPixelFormat = pixFmt;

  encodeSessionExParams.device = this->NvEncDeviceHandle;
  encodeSessionExParams.deviceType = this->NvEncDeviceType;
  encodeSessionExParams.apiVersion = NVENCAPI_VERSION;

  bool success = true;
  NVENCSTATUS errorCode;
  void* session = nullptr;
  VTK_NVENC_API_CHECKED_INVOKE(
    this->NvEncInstance.nvEncOpenEncodeSessionEx(&encodeSessionExParams, &session));
  if (!success)
  {
    vtkLog(ERROR, << "Failed to open an encode session.");
  }
  else
  {
    this->NvEncSession = session;
  }
  return success;
}

//------------------------------------------------------------------------------
bool vtkNvEncoderInternals::LoadNvEncodeAPI()
{
  vtkLogScopeFunction(TRACE);
  bool success = true;
  uint32_t version = 0;
  uint32_t currentVersion = (NVENCAPI_MAJOR_VERSION << 4) | NVENCAPI_MINOR_VERSION;
  NVENCSTATUS errorCode;
  VTK_NVENC_API_CHECKED_INVOKE(this->NvEncodeLoader.NvEncodeAPIGetMaxSupportedVersion(&version));
  if (!success)
  {
    return false;
  }
  if (currentVersion > version)
  {
    vtkLog(ERROR, << "Current driver version does not support NvEncodeAPI-"
                  << NVENCAPI_MAJOR_VERSION << "." << NVENCAPI_MINOR_VERSION
                  << ". Please upgrade your NVIDIA drivers.");
  }

  this->NvEncInstance = { NV_ENCODE_API_FUNCTION_LIST_VER };
  VTK_NVENC_API_CHECKED_INVOKE(
    this->NvEncodeLoader.NvEncodeAPICreateInstance(&this->NvEncInstance));
  if (!success)
  {
    return false;
  }
  else
  {
    return true;
  }
}

//------------------------------------------------------------------------------
bool vtkNvEncoderInternals::CreateDefaultEncoderInitializeParams(NV_ENC_INITIALIZE_PARAMS* params,
  GUID codec, GUID preset, GUID profile, // NOLINT(bugprone-easily-swappable-parameters)
  NV_ENC_TUNING_INFO tuneInfo)
{
  vtkLogScopeFunction(TRACE);
  if (this->NvEncSession == nullptr)
  {
    vtkLog(ERROR, << "Error (" << NV_ENC_ERR_NO_ENCODE_DEVICE
                  << ") Encoder initialization failed - no encode device.");
    return false;
  }

  if (params == nullptr || params->encodeConfig == nullptr)
  {
    vtkLog(ERROR, << "Error (" << NV_ENC_ERR_INVALID_PTR
                  << ") Both params and params->encodeConfig can't be nullptr.");
    return false;
  }

  memset(params->encodeConfig, 0, sizeof(NV_ENC_CONFIG));
  auto encodeConfig = params->encodeConfig;
  memset(params, 0, sizeof(NV_ENC_INITIALIZE_PARAMS));
  params->encodeConfig = encodeConfig;

  params->encodeConfig->version = NV_ENC_CONFIG_VER;
  params->version = NV_ENC_INITIALIZE_PARAMS_VER;
  params->encodeGUID = codec;
  params->presetGUID = preset;
  params->encodeWidth = this->Width;
  params->encodeHeight = this->Height;
  params->darWidth = this->Width;
  params->darHeight = this->Height;
  params->frameRateNum = 30;
  params->frameRateDen = 1;
  params->enablePTD = 1;
  params->reportSliceOffsets = 0;
  params->enableSubFrameWrite = 0;
  params->maxEncodeWidth = this->Width;
  params->maxEncodeHeight = this->Height;
  // we don't support motion-estimation only mode. Use case seems very remote.
  params->enableMEOnlyMode = false;
  params->enableOutputInVidmem = false;

  NV_ENC_PRESET_CONFIG presetConfig = { NV_ENC_PRESET_CONFIG_VER, { NV_ENC_CONFIG_VER } };
  this->NvEncInstance.nvEncGetEncodePresetConfig(this->NvEncSession, codec, preset, &presetConfig);
  memcpy(params->encodeConfig, &presetConfig.presetCfg, sizeof(NV_ENC_CONFIG));
  params->encodeConfig->frameIntervalP = 1;
  params->encodeConfig->gopLength = NVENC_INFINITE_GOPLENGTH;
  params->encodeConfig->rcParams.rateControlMode = NV_ENC_PARAMS_RC_CBR;
  params->encodeConfig->profileGUID = profile;

  // Apply tune information.
  {
    params->tuningInfo = tuneInfo;
    NV_ENC_PRESET_CONFIG presetConfig = { NV_ENC_PRESET_CONFIG_VER, { NV_ENC_CONFIG_VER } };
    this->NvEncInstance.nvEncGetEncodePresetConfigEx(
      this->NvEncSession, codec, preset, tuneInfo, &presetConfig);
    memcpy(params->encodeConfig, &presetConfig.presetCfg, sizeof(NV_ENC_CONFIG));
  }

  // codec specific parameters.
  if (params->encodeGUID == NV_ENC_CODEC_H264_GUID)
  {
    if (this->NvEncPixelFormat == NV_ENC_BUFFER_FORMAT_YUV444 ||
      this->NvEncPixelFormat == NV_ENC_BUFFER_FORMAT_YUV444_10BIT)
    {
      params->encodeConfig->encodeCodecConfig.h264Config.chromaFormatIDC = 3;
    }
    params->encodeConfig->encodeCodecConfig.h264Config.level = NV_ENC_LEVEL_AUTOSELECT;
    params->encodeConfig->encodeCodecConfig.h264Config.idrPeriod = params->encodeConfig->gopLength;
  }
  else if (params->encodeGUID == NV_ENC_CODEC_HEVC_GUID)
  {
    if (this->NvEncPixelFormat == NV_ENC_BUFFER_FORMAT_YUV420_10BIT ||
      this->NvEncPixelFormat == NV_ENC_BUFFER_FORMAT_YUV444_10BIT)
    {
      params->encodeConfig->encodeCodecConfig.hevcConfig.pixelBitDepthMinus8 = 2;
    }
    else
    {
      params->encodeConfig->encodeCodecConfig.hevcConfig.pixelBitDepthMinus8 = 0;
    }
    if (this->NvEncPixelFormat == NV_ENC_BUFFER_FORMAT_YUV444 ||
      this->NvEncPixelFormat == NV_ENC_BUFFER_FORMAT_YUV444_10BIT)
    {
      params->encodeConfig->encodeCodecConfig.hevcConfig.chromaFormatIDC = 3;
    }
    params->encodeConfig->encodeCodecConfig.hevcConfig.idrPeriod = params->encodeConfig->gopLength;
  }

  return true;
}

//------------------------------------------------------------------------------
bool vtkNvEncoderInternals::InitializeEncodeCtx(
  const NV_ENC_INITIALIZE_PARAMS* params, std::size_t extra_delay /*=0*/)
{
  vtkLogScopeFunction(TRACE);
  if (this->NvEncSession == nullptr)
  {
    vtkLog(ERROR, << "Error (" << NV_ENC_ERR_NO_ENCODE_DEVICE
                  << ") Encoder initialization failed - no encode device.");
    return false;
  }

  if (params == nullptr)
  {
    vtkLog(ERROR, << "Error (" << NV_ENC_ERR_INVALID_PTR
                  << ") Encoder initialization failed - no encode device.");
    return false;
  }

  if (params->encodeWidth == 0 || params->encodeHeight == 0)
  {
    vtkLog(
      ERROR, << "Error (" << NV_ENC_ERR_INVALID_PARAM << ") Invalid encoder width and height.");
    return false;
  }

  if (params->encodeGUID != NV_ENC_CODEC_H264_GUID && params->encodeGUID != NV_ENC_CODEC_HEVC_GUID)
  {
    vtkLog(ERROR, << "Error (" << NV_ENC_ERR_INVALID_PARAM
                  << ") Invalid codec id. Supported codecs - H264, HEVC");
    return false;
  }

  // 1. Populate initialization parameters.
  memcpy(&this->NvEncInitializeParams, params, sizeof(NV_ENC_INITIALIZE_PARAMS));
  this->NvEncInitializeParams.version = NV_ENC_INITIALIZE_PARAMS_VER;

  // 2. Populate encoder configuration with provided encoder configuration or invent a default.
  if (params->encodeConfig)
  {
    memcpy(&this->NvEncConfig, params->encodeConfig, sizeof(NV_ENC_CONFIG));
    this->NvEncConfig.version = NV_ENC_CONFIG_VER;
  }
  else
  {
    this->NvEncConfig.version = NV_ENC_CONFIG_VER;
    this->NvEncConfig.frameIntervalP = 1;
    this->NvEncConfig.gopLength = NVENC_INFINITE_GOPLENGTH;
    this->NvEncConfig.rcParams.rateControlMode = NV_ENC_PARAMS_RC_CBR;
  }
  this->NvEncInitializeParams.encodeConfig = &this->NvEncConfig;

  // 3. Sanitize dimensions. 4:2:0 pixel formats subsample chroma 2x2, so NVENC requires even
  // luma dimensions. Odd sizes show up when clients scale by a fractional devicePixelRatio;
  // align down (the registered input buffers may be larger, NVENC crops the extra row/column).
  switch (this->NvEncPixelFormat)
  {
    case NV_ENC_BUFFER_FORMAT_NV12:
    case NV_ENC_BUFFER_FORMAT_YV12:
    case NV_ENC_BUFFER_FORMAT_IYUV:
    case NV_ENC_BUFFER_FORMAT_YUV420_10BIT:
    {
      auto& initParams = this->NvEncInitializeParams;
      initParams.encodeWidth &= ~1U;
      initParams.encodeHeight &= ~1U;
      initParams.darWidth &= ~1U;
      initParams.darHeight &= ~1U;
      initParams.maxEncodeWidth &= ~1U;
      initParams.maxEncodeHeight &= ~1U;
      break;
    }
    default:
      break;
  }

  // 4. Verify the dimensions against the hardware limits for the chosen codec so that failure
  // is actionable instead of a bare NV_ENC_ERR_INVALID_PARAM. Ex: H.264 NVENC caps out at
  // 4096x4096 while HEVC allows 8192x8192.
  {
    const auto codec = this->NvEncInitializeParams.encodeGUID;
    const auto minWidth =
      static_cast<uint32_t>(std::max(0, this->GetCapability(codec, NV_ENC_CAPS_WIDTH_MIN)));
    const auto minHeight =
      static_cast<uint32_t>(std::max(0, this->GetCapability(codec, NV_ENC_CAPS_HEIGHT_MIN)));
    const auto maxWidth =
      static_cast<uint32_t>(std::max(0, this->GetCapability(codec, NV_ENC_CAPS_WIDTH_MAX)));
    const auto maxHeight =
      static_cast<uint32_t>(std::max(0, this->GetCapability(codec, NV_ENC_CAPS_HEIGHT_MAX)));
    const auto& encodeWidth = this->NvEncInitializeParams.encodeWidth;
    const auto& encodeHeight = this->NvEncInitializeParams.encodeHeight;
    if ((maxWidth > 0 && encodeWidth > maxWidth) || (maxHeight > 0 && encodeHeight > maxHeight))
    {
      vtkLog(ERROR, << "Error (" << NV_ENC_ERR_INVALID_PARAM << ") Encode dimensions "
                    << encodeWidth << "x" << encodeHeight << " exceed the NVENC limit of "
                    << maxWidth << "x" << maxHeight
                    << " for the chosen codec. Reduce the frame size or switch to a codec with "
                       "higher limits (ex: HEVC).");
      return false;
    }
    if (encodeWidth < minWidth || encodeHeight < minHeight)
    {
      vtkLog(ERROR, << "Error (" << NV_ENC_ERR_INVALID_PARAM << ") Encode dimensions "
                    << encodeWidth << "x" << encodeHeight << " are below the NVENC minimum of "
                    << minWidth << "x" << minHeight << " for the chosen codec.");
      return false;
    }
  }

  // 5. Initialize the NVENC encoder.
  bool success = true;
  NVENCSTATUS errorCode;
  VTK_NVENC_API_CHECKED_INVOKE(
    this->NvEncInstance.nvEncInitializeEncoder(this->NvEncSession, &this->NvEncInitializeParams));
  this->NvEncInitialized = success;
  if (!success)
  {
    return false;
  }

  // 6. Keep track of the dimensions.
  this->Width = this->NvEncInitializeParams.encodeWidth;
  this->Height = this->NvEncInitializeParams.encodeHeight;
  this->MaxWidth = this->NvEncInitializeParams.maxEncodeWidth;
  this->MaxHeight = this->NvEncInitializeParams.maxEncodeHeight;

  // 7. Compute the number of input/output buffers needed.
  this->NvEncExtraOutputDelay = extra_delay;
  this->NvEncBufferCount = this->NvEncConfig.frameIntervalP +
    this->NvEncConfig.rcParams.lookaheadDepth + this->NvEncExtraOutputDelay;
  if (!this->NvEncBufferCount)
  {
    this->NvEncBufferCount = 1;
  }
  this->NvEncOutputDelay = this->NvEncBufferCount - 1;

  // 8. resize input/output buffers.
  this->NvEncMappedInputBuffers.resize(this->NvEncBufferCount, nullptr);
  this->NvBitstreamBuffers.resize(this->NvEncBufferCount, nullptr);
  this->InitializeBitstreamBuffers();

  return success;
}

//------------------------------------------------------------------------------
bool vtkNvEncoderInternals::RegisterInputResources(const std::vector<void*>& inputResources,
  std::vector<vtkSmartPointer<vtkRawVideoFrame>>& inputFrames,
  NV_ENC_INPUT_RESOURCE_TYPE resourceType, NV_ENC_BUFFER_FORMAT bufferFormat)
{
  vtkLogScopeFunction(TRACE);
  for (std::size_t i = 0; i < inputResources.size(); ++i)
  {
    const auto& width = inputFrames[i]->GetWidth();
    const auto& height = inputFrames[i]->GetStorageHeight();
    const auto& pitch = inputFrames[i]->GetStrides()[0];
    NV_ENC_REGISTERED_PTR registeredPtr = RegisterResource(
      inputResources[i], resourceType, width, height, pitch, bufferFormat, NV_ENC_INPUT_IMAGE);
    this->NvEncRegisteredResources.push_back(registeredPtr);
    this->NvEncInputFrames.push_back(inputFrames[i]);
    this->NvEncInputResources.push_back(inputResources[i]);
  }
  this->NvEncBufferCount = inputResources.size();
  inputFrames.clear();
  return true;
}

//------------------------------------------------------------------------------
NV_ENC_REGISTERED_PTR vtkNvEncoderInternals::RegisterResource(void* resource,
  NV_ENC_INPUT_RESOURCE_TYPE resourceType, int width, int height, int pitch, // NOLINT
  NV_ENC_BUFFER_FORMAT bufferFormat,                                         // NOLINT
  NV_ENC_BUFFER_USAGE bufferUsage /*= NV_ENC_INPUT_IMAGE*/,
  NV_ENC_FENCE_POINT_D3D12* inputFencePoint /*= nullptr*/, // NOLINT
  NV_ENC_FENCE_POINT_D3D12* outputFencePoint /*= nullptr*/)
{
  vtkLogScopeFunction(TRACE);
  NV_ENC_REGISTER_RESOURCE registerResource = { NV_ENC_REGISTER_RESOURCE_VER };
  registerResource.resourceType = resourceType;
  registerResource.resourceToRegister = resource;
  registerResource.width = width;
  registerResource.height = height;
  registerResource.pitch = pitch;
  registerResource.bufferFormat = bufferFormat;
  registerResource.bufferUsage = bufferUsage;
  registerResource.pInputFencePoint = inputFencePoint;
  registerResource.pOutputFencePoint = outputFencePoint;

  bool success = true;
  NVENCSTATUS errorCode;
  VTK_NVENC_API_CHECKED_INVOKE(
    this->NvEncInstance.nvEncRegisterResource(this->NvEncSession, &registerResource));

  if (!success)
  {
    abort();
    vtkLog(ERROR, << "Failed to register resource with NvEncodeAPI.");
    return nullptr;
  }

  return registerResource.registeredResource;
}

//------------------------------------------------------------------------------
bool vtkNvEncoderInternals::MapInputBufferToDevice(std::size_t bfrIdx)
{
  vtkLogScopeFunction(TRACE);
  bool success = true;
  NVENCSTATUS errorCode;
  NV_ENC_MAP_INPUT_RESOURCE mappedInput = { NV_ENC_MAP_INPUT_RESOURCE_VER };
  mappedInput.registeredResource = this->NvEncRegisteredResources[bfrIdx];

  VTK_NVENC_API_CHECKED_INVOKE(
    this->NvEncInstance.nvEncMapInputResource(this->NvEncSession, &mappedInput));

  if (success)
  {
    this->NvEncMappedInputBuffers[bfrIdx] = mappedInput.mappedResource;
  }
  return success;
}

//------------------------------------------------------------------------------
bool vtkNvEncoderInternals::InitializeBitstreamBuffers()
{
  vtkLogScopeFunction(TRACE);
  bool success = true;
  NVENCSTATUS errorCode;
  for (std::size_t i = 0; i < this->NvEncBufferCount; ++i)
  {
    NV_ENC_CREATE_BITSTREAM_BUFFER createBitstreamBuf = { NV_ENC_CREATE_BITSTREAM_BUFFER_VER };

    VTK_NVENC_API_CHECKED_INVOKE(
      this->NvEncInstance.nvEncCreateBitstreamBuffer(this->NvEncSession, &createBitstreamBuf));
    if (!success)
    {
      break;
    }
    this->NvBitstreamBuffers[i] = createBitstreamBuf.bitstreamBuffer;
  }
  return success;
}

//------------------------------------------------------------------------------
bool vtkNvEncoderInternals::ReleaseBitstreamBuffers()
{
  vtkLogScopeFunction(TRACE);
  bool success = true;
  NVENCSTATUS errorCode;
  for (std::size_t i = 0; i < this->NvBitstreamBuffers.size(); ++i)
  {
    if (this->NvBitstreamBuffers[i] != nullptr)
    {
      VTK_NVENC_API_CHECKED_INVOKE(this->NvEncInstance.nvEncDestroyBitstreamBuffer(
        this->NvEncSession, this->NvBitstreamBuffers[i]));
    }
    if (!success)
    {
      break;
    }
  }
  return success;
}

//------------------------------------------------------------------------------
NVENCSTATUS vtkNvEncoderInternals::Send(bool keyFrame /*=false*/)
{
  vtkLogScopeF(TRACE, "%s, %lu, keyFrame=%d", __func__, this->NvEncSendCounter, keyFrame);
  const auto bfrIdx = this->NvEncSendCounter % this->NvEncBufferCount;
  this->MapInputBufferToDevice(bfrIdx);
  vtkLogF(TRACE, "b|s|c| = %lu|%lu|%lu", bfrIdx, this->NvEncSendCounter, this->NvEncBufferCount);

  NV_ENC_INPUT_PTR inputBuffer = this->NvEncMappedInputBuffers[bfrIdx];
  NV_ENC_OUTPUT_PTR bitstreamBuffer = this->NvBitstreamBuffers[bfrIdx];

  NV_ENC_PIC_PARAMS picParams = {};
  picParams.version = NV_ENC_PIC_PARAMS_VER;
  picParams.pictureStruct = NV_ENC_PIC_STRUCT_FRAME;
  picParams.inputBuffer = inputBuffer;
  picParams.bufferFmt = this->NvEncPixelFormat;
  picParams.inputWidth = this->Width;
  picParams.inputHeight = this->Height;
  picParams.outputBitstream = bitstreamBuffer;
  picParams.completionEvent = nullptr;
  if (keyFrame)
  {
    picParams.encodePicFlags = (this->NvEncInitializeParams.enablePTD == 1)
      ? NV_ENC_PIC_FLAG_FORCEIDR
      : NV_ENC_PIC_FLAG_FORCEINTRA;
  }
  else
  {
    picParams.encodePicFlags = NV_ENC_PIC_FLAG_OUTPUT_SPSPPS;
  }
  bool success = true;
  NVENCSTATUS errorCode;
  VTK_NVENC_API_CHECKED_INVOKE(
    this->NvEncInstance.nvEncEncodePicture(this->NvEncSession, &picParams));

  if (errorCode == NV_ENC_SUCCESS || errorCode == NV_ENC_ERR_NEED_MORE_INPUT)
  {
    ++this->NvEncSendCounter;
  }
  vtkLogF(TRACE, "Send status: %s",
    vtkVideoProcessingStatusTypeUtilities::ToString(
      vtkNvEncoderInternals::ParseNvEncodeAPIStatus(errorCode)));
  return errorCode;
}

//------------------------------------------------------------------------------
bool vtkNvEncoderInternals::SendEOS()
{
  vtkLogScopeFunction(TRACE);
  NV_ENC_PIC_PARAMS picParams = { NV_ENC_PIC_PARAMS_VER };
  picParams.encodePicFlags = NV_ENC_PIC_FLAG_EOS;
  picParams.completionEvent = nullptr;
  bool success;
  NVENCSTATUS errorCode;
  VTK_NVENC_API_CHECKED_INVOKE(
    this->NvEncInstance.nvEncEncodePicture(this->NvEncSession, &picParams));
  return success;
}

//------------------------------------------------------------------------------
bool vtkNvEncoderInternals::Receive(
  std::vector<vtkSmartPointer<vtkCompressedVideoPacket>>& packets, bool outputDelay /*=false*/)
{
  vtkLogScopeF(TRACE, "%s, %lu, outputDelay=%s|%lu", __func__, this->NvEncSendCounter - 1,
    (outputDelay ? "true" : "false"), this->NvEncOutputDelay);
  std::size_t iPkt = 0;
  bool success = true;
  NVENCSTATUS errorCode;
  int iEnd = outputDelay ? this->NvEncSendCounter - this->NvEncOutputDelay : this->NvEncSendCounter;

  for (; this->NvEncRecvCounter < iEnd; ++this->NvEncRecvCounter)
  {
    vtkLogF(TRACE, "p|r|c| = %lu|%lu|%lu", iPkt, this->NvEncRecvCounter, this->NvEncBufferCount);
    NV_ENC_LOCK_BITSTREAM lockBitStreamData = { NV_ENC_LOCK_BITSTREAM_VER };
    const auto bfrIdx = this->NvEncRecvCounter % this->NvEncBufferCount;
    lockBitStreamData.outputBitstream = this->NvBitstreamBuffers[bfrIdx];
    lockBitStreamData.doNotWait = false;
    VTK_NVENC_API_CHECKED_INVOKE(
      this->NvEncInstance.nvEncLockBitstream(this->NvEncSession, &lockBitStreamData));

    vtkLogF(TRACE, "Recv status: %s",
      vtkVideoProcessingStatusTypeUtilities::ToString(
        vtkNvEncoderInternals::ParseNvEncodeAPIStatus(errorCode)));
    if (!success)
    {
      return false;
    }

    unsigned char* data = reinterpret_cast<unsigned char*>(lockBitStreamData.bitstreamBufferPtr);
    if (packets.size() < iPkt + 1)
    {
      packets.emplace_back(vtkSmartPointer<vtkCompressedVideoPacket>::New());
    }
    packets[iPkt]->SetDisplayWidth(this->Width);
    packets[iPkt]->SetDisplayHeight(this->Height);
    // Key frames carry the sequence header; parse the codec string from it and
    // cache it so the following delta frames report the same value.
    const bool keyFrame = lockBitStreamData.pictureType == NV_ENC_PIC_TYPE_IDR ||
      lockBitStreamData.pictureType == NV_ENC_PIC_TYPE_I;
    packets[iPkt]->SetIsKeyFrame(keyFrame);
    if (keyFrame || this->CodecName.empty())
    {
      std::string parsed = BuildCodecString(
        this->NvEncInitializeParams.encodeGUID, data, lockBitStreamData.bitstreamSizeInBytes);
      if (!parsed.empty())
      {
        this->CodecName = std::move(parsed);
      }
    }
    packets[iPkt]->SetCodecLongName(this->CodecName.c_str());
    // The coded picture dimensions are aligned to the codec's coding-block size:
    // H.264 codes in 16x16 macroblocks, while HEVC signals a coded size that is a
    // multiple of the minimum luma coding block size (8). Anything smaller here would
    // under-report the coded size for H.264.
    const int codedAlign =
      (this->NvEncInitializeParams.encodeGUID == NV_ENC_CODEC_H264_GUID) ? 16 : 8;
    packets[iPkt]->SetCodedWidth(vtkRawVideoFrame::AlignUp(this->Width, codedAlign));
    packets[iPkt]->SetCodedHeight(vtkRawVideoFrame::AlignUp(this->Height, codedAlign));
    packets[iPkt]->SetPresentationTS(this->NvEncRecvCounter);
    packets[iPkt]->SetSize(lockBitStreamData.bitstreamSizeInBytes);
    packets[iPkt]->CopyData(data, lockBitStreamData.bitstreamSizeInBytes);
    ++iPkt;

    VTK_NVENC_API_CHECKED_INVOKE(this->NvEncInstance.nvEncUnlockBitstream(
      this->NvEncSession, lockBitStreamData.outputBitstream));
    if (!success)
    {
      return false;
    }

    if (this->NvEncMappedInputBuffers[bfrIdx] != nullptr)
    {
      VTK_NVENC_API_CHECKED_INVOKE(this->NvEncInstance.nvEncUnmapInputResource(
        this->NvEncSession, this->NvEncMappedInputBuffers[bfrIdx]));
      if (!success)
      {
        return false;
      }
      this->NvEncMappedInputBuffers[bfrIdx] = nullptr;
    }
  }
  return success;
}

//------------------------------------------------------------------------------
NVENCSTATUS vtkNvEncoderInternals::Encode(
  std::vector<vtkSmartPointer<vtkCompressedVideoPacket>>& packets, bool keyFrame /*=false*/,
  bool outputDelay /*=false*/)
{
  vtkLogScopeFunction(TRACE);
  packets.clear();
  if (!this->IsInitialized())
  {
    vtkLog(ERROR, << "Error (" << NV_ENC_ERR_NO_ENCODE_DEVICE
                  << ") Encoder initialization failed - no encode device.");
    return NV_ENC_ERR_NO_ENCODE_DEVICE;
  }

  NVENCSTATUS status = this->Send(keyFrame);
  if (status == NV_ENC_SUCCESS || status == NV_ENC_ERR_NEED_MORE_INPUT)
  {
    this->Receive(packets, true);
  }
  else
  {
    vtkLog(ERROR, << "Error (" << status << ") nvEncEncodePicture API failed.");
  }
  return status;
}

//------------------------------------------------------------------------------
int vtkNvEncoderInternals::GetCapability(GUID guidCodec, NV_ENC_CAPS caps)
{
  vtkLogScopeFunction(TRACE);
  if (this->NvEncSession == nullptr)
  {
    return 0;
  }

  NV_ENC_CAPS_PARAM capsParam = { NV_ENC_CAPS_PARAM_VER };
  capsParam.capsToQuery = caps;
  int value;
  this->NvEncInstance.nvEncGetEncodeCaps(this->NvEncSession, guidCodec, &capsParam, &value);
  return value;
}

//------------------------------------------------------------------------------
void vtkNvEncoderInternals::GetInitializeParams(NV_ENC_INITIALIZE_PARAMS* params)
{
  vtkLogScopeFunction(TRACE);
  if (params == nullptr || params->encodeConfig == nullptr)
  {
    vtkLog(ERROR, << "Error (" << NV_ENC_ERR_INVALID_PTR
                  << ") Both params and params->encodeConfig can't be nullptr.");
    return;
  }
  NV_ENC_CONFIG* encodeConfig = params->encodeConfig;
  *encodeConfig = this->NvEncConfig;
  *params = this->NvEncInitializeParams;
  params->encodeConfig = encodeConfig;
}

//------------------------------------------------------------------------------
vtkSmartPointer<vtkRawVideoFrame> vtkNvEncoderInternals::GetNextInputFrame() const
{
  vtkLogScopeFunction(TRACE);
  const auto bfrIdx = this->NvEncSendCounter % this->NvEncBufferCount;
  return this->NvEncInputFrames[bfrIdx];
}

//------------------------------------------------------------------------------
bool vtkNvEncoderInternals::EndEncode(
  std::vector<vtkSmartPointer<vtkCompressedVideoPacket>>& packets)
{
  vtkLogScopeFunction(TRACE);
  packets.clear();
  if (!this->IsInitialized())
  {
    return true;
  }
  if (this->SendEOS())
  {
    return this->Receive(packets, /*outputDelay=*/false);
  }
  else
  {
    return false;
  }
}

//------------------------------------------------------------------------------
void vtkNvEncoderInternals::Flush()
{
  vtkLogScopeFunction(TRACE);
  try
  {
    // Incase of error it is possible the encoder still has buffers mapped.
    // flush the encoder queue and then unmapp it if any surface is still mapped
    std::vector<vtkSmartPointer<vtkCompressedVideoPacket>> packets;
    this->EndEncode(packets);
  }
  catch (...)
  {
  }
}

//------------------------------------------------------------------------------
void vtkNvEncoderInternals::UnregisterInputResources()
{
  vtkLogScopeFunction(TRACE);
  this->Flush();
  bool success;
  NVENCSTATUS errorCode;
  // 1. unmap input buffers.
  for (std::size_t i = 0; i < this->NvEncMappedInputBuffers.size(); ++i)
  {
    if (this->NvEncMappedInputBuffers[i] != nullptr)
    {
      VTK_NVENC_API_CHECKED_INVOKE(this->NvEncInstance.nvEncUnmapInputResource(
        this->NvEncSession, this->NvEncMappedInputBuffers[i]));
    }
  }
  this->NvEncMappedInputBuffers.clear();

  // 2. unregister resources.
  for (std::size_t i = 0; i < this->NvEncRegisteredResources.size(); ++i)
  {
    if (this->NvEncRegisteredResources[i] != nullptr)
    {
      VTK_NVENC_API_CHECKED_INVOKE(this->NvEncInstance.nvEncUnregisterResource(
        this->NvEncSession, this->NvEncRegisteredResources[i]));
    }
  }
  this->NvEncRegisteredResources.clear();
  this->NvEncBufferCount = 0;
}

//------------------------------------------------------------------------------
bool vtkNvEncoderInternals::Shutdown()
{
  vtkLogScopeFunction(TRACE);
  if (!this->IsInitialized())
  {
    return true;
  }

  if (!this->ReleaseBitstreamBuffers())
  {
    return false;
  }

  bool success = true;
  NVENCSTATUS errorCode;
  VTK_NVENC_API_CHECKED_INVOKE(this->NvEncInstance.nvEncDestroyEncoder(this->NvEncSession));
  if (success)
  {
    this->NvEncInitialized = false;
    this->NvEncSession = nullptr;
  }

  return success;
}

//------------------------------------------------------------------------------
VTKVideoProcessingStatusType vtkNvEncoderInternals::ParseNvEncodeAPIStatus(NVENCSTATUS statusCode)
{

  VTKVideoProcessingStatusType result;
  switch (statusCode)
  {
    case NV_ENC_SUCCESS:
      result = VTKVideoProcessingStatusType::VTKVPStatus_Success;
      break;
    case NV_ENC_ERR_NO_ENCODE_DEVICE:
    case NV_ENC_ERR_UNSUPPORTED_DEVICE:
    case NV_ENC_ERR_INVALID_ENCODERDEVICE:
    case NV_ENC_ERR_INVALID_DEVICE:
    case NV_ENC_ERR_DEVICE_NOT_EXIST:
      // logs will have error code, so let's just simply treat it as unknown here.
      result = VTKVideoProcessingStatusType::VTKVPStatus_UnknownError;
      break;
    case NV_ENC_ERR_INVALID_PTR:
    case NV_ENC_ERR_INVALID_EVENT:
    case NV_ENC_ERR_INVALID_PARAM:
    case NV_ENC_ERR_INVALID_CALL:
      result = VTKVideoProcessingStatusType::VTKVPStatus_InvalidValue;
      break;
    case NV_ENC_ERR_OUT_OF_MEMORY:
      result = VTKVideoProcessingStatusType::VTKVPStatus_OutOfMemory;
      break;
    case NV_ENC_ERR_ENCODER_NOT_INITIALIZED:
    case NV_ENC_ERR_UNSUPPORTED_PARAM:
      result = VTKVideoProcessingStatusType::VTKVPStatus_InvalidValue;
      break;
    case NV_ENC_ERR_LOCK_BUSY:
      result = VTKVideoProcessingStatusType::VTKVPStatus_Busy;
      break;
    case NV_ENC_ERR_NOT_ENOUGH_BUFFER:
      result = VTKVideoProcessingStatusType::VTKVPStatus_InvalidValue;
      break;
    case NV_ENC_ERR_INVALID_VERSION:
      result = VTKVideoProcessingStatusType::VTKVPStatus_InvalidValue;
      break;
    case NV_ENC_ERR_MAP_FAILED:
      // logs will have error code, so let's just simply treat it as unknown here.
      result = VTKVideoProcessingStatusType::VTKVPStatus_UnknownError;
      break;
    case NV_ENC_ERR_NEED_MORE_INPUT:
      result = VTKVideoProcessingStatusType::VTKVPStatus_TrySendAgain;
      break;
    case NV_ENC_ERR_ENCODER_BUSY:
      result = VTKVideoProcessingStatusType::VTKVPStatus_Busy;
      break;
    case NV_ENC_ERR_EVENT_NOT_REGISTERD:
      result = VTKVideoProcessingStatusType::VTKVPStatus_InvalidValue;
      break;
    case NV_ENC_ERR_GENERIC:
      // logs will have error code, so let's just simply treat it as unknown here.
      result = VTKVideoProcessingStatusType::VTKVPStatus_UnknownError;
      break;
    case NV_ENC_ERR_INCOMPATIBLE_CLIENT_KEY:
      result = VTKVideoProcessingStatusType::VTKVPStatus_InvalidValue;
      break;
    case NV_ENC_ERR_UNIMPLEMENTED:
    case NV_ENC_ERR_RESOURCE_REGISTER_FAILED:
    case NV_ENC_ERR_RESOURCE_NOT_REGISTERED:
    case NV_ENC_ERR_RESOURCE_NOT_MAPPED:
      // logs will have error code, so let's just simply treat it as unknown here.
      result = VTKVideoProcessingStatusType::VTKVPStatus_UnknownError;
      break;
  }
  return result;
}

//------------------------------------------------------------------------------
VTKPixelFormatType vtkNvEncoderInternals::ParsePixelFormat(NV_ENC_BUFFER_FORMAT nvEncBufFmt)
{
  switch (nvEncBufFmt)
  {
    case NV_ENC_BUFFER_FORMAT_NV12:
      return VTKPixelFormatType::VTKPF_NV12;
    case NV_ENC_BUFFER_FORMAT_IYUV:
      return VTKPixelFormatType::VTKPF_IYUV;
    case NV_ENC_BUFFER_FORMAT_ABGR:
      return VTKPixelFormatType::VTKPF_RGBA32;
    default:
      return VTKPixelFormatType::VTKPF_NV12;
  }
}

//------------------------------------------------------------------------------
NV_ENC_BUFFER_FORMAT vtkNvEncoderInternals::ParsePixelFormat(VTKPixelFormatType vtkPixFmt)
{
  switch (vtkPixFmt)
  {
    case VTKPixelFormatType::VTKPF_NV12:
      return NV_ENC_BUFFER_FORMAT_NV12;
    case VTKPixelFormatType::VTKPF_IYUV:
      return NV_ENC_BUFFER_FORMAT_IYUV;
    case VTKPixelFormatType::VTKPF_RGBA32:
      return NV_ENC_BUFFER_FORMAT_ABGR;
    default:
      return NV_ENC_BUFFER_FORMAT_NV12;
  }
}

//------------------------------------------------------------------------------
bool vtkNvEncoderInternals::TweakFromEncoderObject(
  NV_ENC_INITIALIZE_PARAMS* params, vtkVideoEncoder* encoderObject)
{
  NV_ENC_CONFIG* encodeConfig = params->encodeConfig;
  // 1. Codec type.
  switch (encoderObject->GetCodec())
  {
    case VTKVideoCodecType::VTKVC_AV1:
    case VTKVideoCodecType::VTKVC_VP9:
      vtkLogF(ERROR, "%s is not supported by NVENC.",
        vtkVideoCodecTypeUtilities::ToString(encoderObject->GetCodec()));
      encoderObject->SetCodec(VTKVideoCodecType::VTKVC_H264);
      params->encodeGUID = NV_ENC_CODEC_H264_GUID;
      break;
    case VTKVideoCodecType::VTKVC_H265:
      params->encodeGUID = NV_ENC_CODEC_HEVC_GUID;
      break;
    case VTKVideoCodecType::VTKVC_H264:
    default:
      params->encodeGUID = NV_ENC_CODEC_H264_GUID;
      break;
  }

  // 2. sequence parameters.
  params->frameRateNum = encoderObject->GetTimeBaseEnd();
  params->frameRateDen = encoderObject->GetTimeBaseStart();
  if (encoderObject->GetLowDelayMode())
  {
    params->encodeConfig->gopLength = NVENC_INFINITE_GOPLENGTH;
    params->encodeConfig->frameIntervalP = 1;
    params->enablePTD = 1;
  }
  else
  {
    params->encodeConfig->gopLength = encoderObject->GetGroupOfPicturesSize();
    params->encodeConfig->frameIntervalP = 2;
    params->enablePTD = 0;
  }
  if (encoderObject->GetKeyFramesOnly())
  {
    params->encodeConfig->gopLength = 0;
    params->encodeConfig->frameIntervalP = 0;
    params->enablePTD = 1;
  }
  else if (encoderObject->GetMaximumBFrames() > 0)
  {
    params->encodeConfig->frameIntervalP = 3;
    params->enablePTD = 0;
  }

  // 3. Picture parameters.
  params->encodeWidth = encoderObject->GetWidth();
  params->encodeHeight = encoderObject->GetHeight();
  params->maxEncodeWidth = encoderObject->GetWidth();
  params->maxEncodeHeight = encoderObject->GetHeight();
  params->bufferFormat = ParsePixelFormat(encoderObject->GetInputPixelFormat());

  // 4. Bitrate control
  const auto q = encoderObject->GetQuantizationParameter();
  if (encoderObject->GetBitRateControlMode() == vtkVideoEncoder::BRCType::CBR)
  {
    params->encodeConfig->rcParams.rateControlMode = NV_ENC_PARAMS_RC_CBR;
    params->encodeConfig->rcParams.averageBitRate = encoderObject->GetBitRate();
    params->encodeConfig->rcParams.enableInitialRCQP = 1;
    params->encodeConfig->rcParams.enableMinQP = 1;
    params->encodeConfig->rcParams.enableMaxQP = 1;
    params->encodeConfig->rcParams.enableAQ = 1;
    params->encodeConfig->rcParams.minQP.qpInterB = q;
    params->encodeConfig->rcParams.minQP.qpInterP = q;
    params->encodeConfig->rcParams.minQP.qpIntra = q;
    params->encodeConfig->rcParams.maxQP.qpInterB = 63;
    params->encodeConfig->rcParams.maxQP.qpInterP = 63;
    params->encodeConfig->rcParams.maxQP.qpIntra = 63;
    vtkLogF(TRACE, "Applied CQP with qp=%d", q);
  }
  else if (encoderObject->GetBitRateControlMode() == vtkVideoEncoder::BRCType::VBR)
  {
    params->encodeConfig->rcParams.rateControlMode = NV_ENC_PARAMS_RC_VBR;
    params->encodeConfig->rcParams.averageBitRate = encoderObject->GetBitRate();
    params->encodeConfig->rcParams.maxBitRate = encoderObject->GetMaxBitRate();
    params->encodeConfig->rcParams.enableInitialRCQP = 1;
    params->encodeConfig->rcParams.enableMinQP = 1;
    params->encodeConfig->rcParams.enableMaxQP = 1;
    params->encodeConfig->rcParams.enableAQ = 1;
    params->encodeConfig->rcParams.minQP.qpInterB = q;
    params->encodeConfig->rcParams.minQP.qpInterP = q;
    params->encodeConfig->rcParams.minQP.qpIntra = q;
    params->encodeConfig->rcParams.maxQP.qpInterB = 63;
    params->encodeConfig->rcParams.maxQP.qpInterP = 63;
    params->encodeConfig->rcParams.maxQP.qpIntra = 63;
    vtkLogF(TRACE, "Applied CQP with qp=%d", q);
  }
  else if (encoderObject->GetBitRateControlMode() == vtkVideoEncoder::BRCType::CQP)
  {
    params->encodeConfig->rcParams.rateControlMode = NV_ENC_PARAMS_RC_CONSTQP;
    vtkLogF(TRACE, "Applied CQP with qp=%d", q);
    params->encodeConfig->rcParams.enableMinQP = 0;
    params->encodeConfig->rcParams.enableMaxQP = 0;
    params->encodeConfig->rcParams.enableAQ = 0;
    params->encodeConfig->rcParams.constQP = { q, q, q };
  }
  else if (encoderObject->GetBitRateControlMode() == vtkVideoEncoder::BRCType::QP)
  {
    params->encodeConfig->rcParams.rateControlMode = NV_ENC_PARAMS_RC_CONSTQP;
    vtkLogF(TRACE, "Applied QP with qp=%d", q);
    params->encodeConfig->rcParams.enableMinQP = 0;
    params->encodeConfig->rcParams.enableMaxQP = 0;
    params->encodeConfig->rcParams.averageBitRate = 0;
    params->encodeConfig->rcParams.maxBitRate = 0;
    params->encodeConfig->rcParams.enableAQ = 0;
    params->encodeConfig->rcParams.constQP = { q, q, q };
  }

  if (encoderObject->GetLowDelayMode())
  {
    params->encodeConfig->rcParams.enableLookahead = 0;
    params->presetGUID = NV_ENC_PRESET_P1_GUID;
    params->tuningInfo = NV_ENC_TUNING_INFO_ULTRA_LOW_LATENCY;
  }

  return true;
}

/*
 * Generates and returns a string describing the values for each field in
 * the NV_ENC_INITIALIZE_PARAMS structure (i.e. a description of the entire
 * set of initialization parameters supplied to the API).
 */
std::string vtkNvEncoderInternals::FullParamToString(
  const NV_ENC_INITIALIZE_PARAMS* pInitializeParams)
{
  std::ostringstream os;
  os << "NV_ENC_INITIALIZE_PARAMS:" << std::endl
     << "encodeGUID: " << ConvertValueToString(vCodec, codecNames, pInitializeParams->encodeGUID)
     << std::endl
     << "presetGUID: "
     << ConvertValueToString(vPreset, szPresetNames, pInitializeParams->presetGUID) << std::endl;
  if (pInitializeParams->tuningInfo)
  {
    os << "tuningInfo: "
       << ConvertValueToString(vTuningInfo, szTuningInfoNames, pInitializeParams->tuningInfo)
       << std::endl;
  }
  os << "encodeWidth: " << pInitializeParams->encodeWidth << std::endl
     << "encodeHeight: " << pInitializeParams->encodeHeight << std::endl
     << "darWidth: " << pInitializeParams->darWidth << std::endl
     << "darHeight: " << pInitializeParams->darHeight << std::endl
     << "frameRateNum: " << pInitializeParams->frameRateNum << std::endl
     << "frameRateDen: " << pInitializeParams->frameRateDen << std::endl
     << "enableEncodeAsync: " << pInitializeParams->enableEncodeAsync << std::endl
     << "reportSliceOffsets: " << pInitializeParams->reportSliceOffsets << std::endl
     << "enableSubFrameWrite: " << pInitializeParams->enableSubFrameWrite << std::endl
     << "enableExternalMEHints: " << pInitializeParams->enableExternalMEHints << std::endl
     << "enableMEOnlyMode: " << pInitializeParams->enableMEOnlyMode << std::endl
     << "enableWeightedPrediction: " << pInitializeParams->enableWeightedPrediction << std::endl
     << "maxEncodeWidth: " << pInitializeParams->maxEncodeWidth << std::endl
     << "maxEncodeHeight: " << pInitializeParams->maxEncodeHeight << std::endl
     << "maxMEHintCountsPerBlock: " << pInitializeParams->maxMEHintCountsPerBlock << std::endl;
  NV_ENC_CONFIG* pConfig = pInitializeParams->encodeConfig;
  os << "NV_ENC_CONFIG:" << std::endl
     << "profile: " << ConvertValueToString(vProfile, szProfileNames, pConfig->profileGUID)
     << std::endl
     << "gopLength: " << pConfig->gopLength << std::endl
     << "frameIntervalP: " << pConfig->frameIntervalP << std::endl
     << "monoChromeEncoding: " << pConfig->monoChromeEncoding << std::endl
     << "frameFieldMode: " << pConfig->frameFieldMode << std::endl
     << "mvPrecision: " << pConfig->mvPrecision << std::endl
     << "NV_ENC_RC_PARAMS:" << std::endl
     << "    rateControlMode: 0x" << std::hex << pConfig->rcParams.rateControlMode << std::dec
     << std::endl
     << "    constQP: " << pConfig->rcParams.constQP.qpInterP << ", "
     << pConfig->rcParams.constQP.qpInterB << ", " << pConfig->rcParams.constQP.qpIntra << std::endl
     << "    averageBitRate:  " << pConfig->rcParams.averageBitRate << std::endl
     << "    maxBitRate:      " << pConfig->rcParams.maxBitRate << std::endl
     << "    vbvBufferSize:   " << pConfig->rcParams.vbvBufferSize << std::endl
     << "    vbvInitialDelay: " << pConfig->rcParams.vbvInitialDelay << std::endl
     << "    enableMinQP: " << pConfig->rcParams.enableMinQP << std::endl
     << "    enableMaxQP: " << pConfig->rcParams.enableMaxQP << std::endl
     << "    enableInitialRCQP: " << pConfig->rcParams.enableInitialRCQP << std::endl
     << "    enableAQ: " << pConfig->rcParams.enableAQ << std::endl
     << "    qpMapMode: "
     << ConvertValueToString(vQpMapMode, szQpMapModeNames, pConfig->rcParams.qpMapMode) << std::endl
     << "    multipass: "
     << ConvertValueToString(vMultiPass, szMultipass, pConfig->rcParams.multiPass) << std::endl
     << "    enableLookahead: " << pConfig->rcParams.enableLookahead << std::endl
     << "    disableIadapt: " << pConfig->rcParams.disableIadapt << std::endl
     << "    disableBadapt: " << pConfig->rcParams.disableBadapt << std::endl
     << "    enableTemporalAQ: " << pConfig->rcParams.enableTemporalAQ << std::endl
     << "    zeroReorderDelay: " << pConfig->rcParams.zeroReorderDelay << std::endl
     << "    enableNonRefP: " << pConfig->rcParams.enableNonRefP << std::endl
     << "    strictGOPTarget: " << pConfig->rcParams.strictGOPTarget << std::endl
     << "    aqStrength: " << pConfig->rcParams.aqStrength << std::endl
     << "    minQP: " << pConfig->rcParams.minQP.qpInterP << ", "
     << pConfig->rcParams.minQP.qpInterB << ", " << pConfig->rcParams.minQP.qpIntra << std::endl
     << "    maxQP: " << pConfig->rcParams.maxQP.qpInterP << ", "
     << pConfig->rcParams.maxQP.qpInterB << ", " << pConfig->rcParams.maxQP.qpIntra << std::endl
     << "    initialRCQP: " << pConfig->rcParams.initialRCQP.qpInterP << ", "
     << pConfig->rcParams.initialRCQP.qpInterB << ", " << pConfig->rcParams.initialRCQP.qpIntra
     << std::endl
     << "    temporallayerIdxMask: " << pConfig->rcParams.temporallayerIdxMask << std::endl
     << "    temporalLayerQP: " << (int)pConfig->rcParams.temporalLayerQP[0] << ", "
     << (int)pConfig->rcParams.temporalLayerQP[1] << ", "
     << (int)pConfig->rcParams.temporalLayerQP[2] << ", "
     << (int)pConfig->rcParams.temporalLayerQP[3] << ", "
     << (int)pConfig->rcParams.temporalLayerQP[4] << ", "
     << (int)pConfig->rcParams.temporalLayerQP[5] << ", "
     << (int)pConfig->rcParams.temporalLayerQP[6] << ", "
     << (int)pConfig->rcParams.temporalLayerQP[7] << std::endl
     << "    targetQuality: " << pConfig->rcParams.targetQuality << std::endl
     << "    lookaheadDepth: " << pConfig->rcParams.lookaheadDepth << std::endl;
  if (pInitializeParams->encodeGUID == NV_ENC_CODEC_H264_GUID)
  {
    os << "NV_ENC_CODEC_CONFIG (H264):" << std::endl
       << "    enableStereoMVC: " << pConfig->encodeCodecConfig.h264Config.enableStereoMVC
       << std::endl
       << "    hierarchicalPFrames: " << pConfig->encodeCodecConfig.h264Config.hierarchicalPFrames
       << std::endl
       << "    hierarchicalBFrames: " << pConfig->encodeCodecConfig.h264Config.hierarchicalBFrames
       << std::endl
       << "    outputBufferingPeriodSEI: "
       << pConfig->encodeCodecConfig.h264Config.outputBufferingPeriodSEI << std::endl
       << "    outputPictureTimingSEI: "
       << pConfig->encodeCodecConfig.h264Config.outputPictureTimingSEI << std::endl
       << "    outputAUD: " << pConfig->encodeCodecConfig.h264Config.outputAUD << std::endl
       << "    disableSPSPPS: " << pConfig->encodeCodecConfig.h264Config.disableSPSPPS << std::endl
       << "    outputFramePackingSEI: "
       << pConfig->encodeCodecConfig.h264Config.outputFramePackingSEI << std::endl
       << "    outputRecoveryPointSEI: "
       << pConfig->encodeCodecConfig.h264Config.outputRecoveryPointSEI << std::endl
       << "    enableIntraRefresh: " << pConfig->encodeCodecConfig.h264Config.enableIntraRefresh
       << std::endl
       << "    enableConstrainedEncoding: "
       << pConfig->encodeCodecConfig.h264Config.enableConstrainedEncoding << std::endl
       << "    repeatSPSPPS: " << pConfig->encodeCodecConfig.h264Config.repeatSPSPPS << std::endl
       << "    enableVFR: " << pConfig->encodeCodecConfig.h264Config.enableVFR << std::endl
       << "    enableLTR: " << pConfig->encodeCodecConfig.h264Config.enableLTR << std::endl
       << "    qpPrimeYZeroTransformBypassFlag: "
       << pConfig->encodeCodecConfig.h264Config.qpPrimeYZeroTransformBypassFlag << std::endl
       << "    useConstrainedIntraPred: "
       << pConfig->encodeCodecConfig.h264Config.useConstrainedIntraPred << std::endl
       << "    level: " << pConfig->encodeCodecConfig.h264Config.level << std::endl
       << "    idrPeriod: " << pConfig->encodeCodecConfig.h264Config.idrPeriod << std::endl
       << "    separateColourPlaneFlag: "
       << pConfig->encodeCodecConfig.h264Config.separateColourPlaneFlag << std::endl
       << "    disableDeblockingFilterIDC: "
       << pConfig->encodeCodecConfig.h264Config.disableDeblockingFilterIDC << std::endl
       << "    numTemporalLayers: " << pConfig->encodeCodecConfig.h264Config.numTemporalLayers
       << std::endl
       << "    spsId: " << pConfig->encodeCodecConfig.h264Config.spsId << std::endl
       << "    ppsId: " << pConfig->encodeCodecConfig.h264Config.ppsId << std::endl
       << "    adaptiveTransformMode: "
       << pConfig->encodeCodecConfig.h264Config.adaptiveTransformMode << std::endl
       << "    fmoMode: " << pConfig->encodeCodecConfig.h264Config.fmoMode << std::endl
       << "    bdirectMode: " << pConfig->encodeCodecConfig.h264Config.bdirectMode << std::endl
       << "    entropyCodingMode: " << pConfig->encodeCodecConfig.h264Config.entropyCodingMode
       << std::endl
       << "    stereoMode: " << pConfig->encodeCodecConfig.h264Config.stereoMode << std::endl
       << "    intraRefreshPeriod: " << pConfig->encodeCodecConfig.h264Config.intraRefreshPeriod
       << std::endl
       << "    intraRefreshCnt: " << pConfig->encodeCodecConfig.h264Config.intraRefreshCnt
       << std::endl
       << "    maxNumRefFrames: " << pConfig->encodeCodecConfig.h264Config.maxNumRefFrames
       << std::endl
       << "    sliceMode: " << pConfig->encodeCodecConfig.h264Config.sliceMode << std::endl
       << "    sliceModeData: " << pConfig->encodeCodecConfig.h264Config.sliceModeData << std::endl
       << "    NV_ENC_CONFIG_H264_VUI_PARAMETERS:" << std::endl
       << "        overscanInfoPresentFlag: "
       << pConfig->encodeCodecConfig.h264Config.h264VUIParameters.overscanInfoPresentFlag
       << std::endl
       << "        overscanInfo: "
       << pConfig->encodeCodecConfig.h264Config.h264VUIParameters.overscanInfo << std::endl
       << "        videoSignalTypePresentFlag: "
       << pConfig->encodeCodecConfig.h264Config.h264VUIParameters.videoSignalTypePresentFlag
       << std::endl
       << "        videoFormat: "
       << pConfig->encodeCodecConfig.h264Config.h264VUIParameters.videoFormat << std::endl
       << "        videoFullRangeFlag: "
       << pConfig->encodeCodecConfig.h264Config.h264VUIParameters.videoFullRangeFlag << std::endl
       << "        colourDescriptionPresentFlag: "
       << pConfig->encodeCodecConfig.h264Config.h264VUIParameters.colourDescriptionPresentFlag
       << std::endl
       << "        colourPrimaries: "
       << pConfig->encodeCodecConfig.h264Config.h264VUIParameters.colourPrimaries << std::endl
       << "        transferCharacteristics: "
       << pConfig->encodeCodecConfig.h264Config.h264VUIParameters.transferCharacteristics
       << std::endl
       << "        colourMatrix: "
       << pConfig->encodeCodecConfig.h264Config.h264VUIParameters.colourMatrix << std::endl
       << "        chromaSampleLocationFlag: "
       << pConfig->encodeCodecConfig.h264Config.h264VUIParameters.chromaSampleLocationFlag
       << std::endl
       << "        chromaSampleLocationTop: "
       << pConfig->encodeCodecConfig.h264Config.h264VUIParameters.chromaSampleLocationTop
       << std::endl
       << "        chromaSampleLocationBot: "
       << pConfig->encodeCodecConfig.h264Config.h264VUIParameters.chromaSampleLocationBot
       << std::endl
       << "        bitstreamRestrictionFlag: "
       << pConfig->encodeCodecConfig.h264Config.h264VUIParameters.bitstreamRestrictionFlag
       << std::endl
       << "    ltrNumFrames: " << pConfig->encodeCodecConfig.h264Config.ltrNumFrames << std::endl
       << "    ltrTrustMode: " << pConfig->encodeCodecConfig.h264Config.ltrTrustMode << std::endl
       << "    chromaFormatIDC: " << pConfig->encodeCodecConfig.h264Config.chromaFormatIDC
       << std::endl
       << "    maxTemporalLayers: " << pConfig->encodeCodecConfig.h264Config.maxTemporalLayers
       << std::endl;
  }
  else if (pInitializeParams->encodeGUID == NV_ENC_CODEC_HEVC_GUID)
  {
    os << "NV_ENC_CODEC_CONFIG (HEVC):" << std::endl
       << "    level: " << pConfig->encodeCodecConfig.hevcConfig.level << std::endl
       << "    tier: " << pConfig->encodeCodecConfig.hevcConfig.tier << std::endl
       << "    minCUSize: " << pConfig->encodeCodecConfig.hevcConfig.minCUSize << std::endl
       << "    maxCUSize: " << pConfig->encodeCodecConfig.hevcConfig.maxCUSize << std::endl
       << "    useConstrainedIntraPred: "
       << pConfig->encodeCodecConfig.hevcConfig.useConstrainedIntraPred << std::endl
       << "    disableDeblockAcrossSliceBoundary: "
       << pConfig->encodeCodecConfig.hevcConfig.disableDeblockAcrossSliceBoundary << std::endl
       << "    outputBufferingPeriodSEI: "
       << pConfig->encodeCodecConfig.hevcConfig.outputBufferingPeriodSEI << std::endl
       << "    outputPictureTimingSEI: "
       << pConfig->encodeCodecConfig.hevcConfig.outputPictureTimingSEI << std::endl
       << "    outputAUD: " << pConfig->encodeCodecConfig.hevcConfig.outputAUD << std::endl
       << "    enableLTR: " << pConfig->encodeCodecConfig.hevcConfig.enableLTR << std::endl
       << "    disableSPSPPS: " << pConfig->encodeCodecConfig.hevcConfig.disableSPSPPS << std::endl
       << "    repeatSPSPPS: " << pConfig->encodeCodecConfig.hevcConfig.repeatSPSPPS << std::endl
       << "    enableIntraRefresh: " << pConfig->encodeCodecConfig.hevcConfig.enableIntraRefresh
       << std::endl
       << "    chromaFormatIDC: " << pConfig->encodeCodecConfig.hevcConfig.chromaFormatIDC
       << std::endl
       << "    pixelBitDepthMinus8: " << pConfig->encodeCodecConfig.hevcConfig.pixelBitDepthMinus8
       << std::endl
       << "    idrPeriod: " << pConfig->encodeCodecConfig.hevcConfig.idrPeriod << std::endl
       << "    intraRefreshPeriod: " << pConfig->encodeCodecConfig.hevcConfig.intraRefreshPeriod
       << std::endl
       << "    intraRefreshCnt: " << pConfig->encodeCodecConfig.hevcConfig.intraRefreshCnt
       << std::endl
       << "    maxNumRefFramesInDPB: " << pConfig->encodeCodecConfig.hevcConfig.maxNumRefFramesInDPB
       << std::endl
       << "    ltrNumFrames: " << pConfig->encodeCodecConfig.hevcConfig.ltrNumFrames << std::endl
       << "    vpsId: " << pConfig->encodeCodecConfig.hevcConfig.vpsId << std::endl
       << "    spsId: " << pConfig->encodeCodecConfig.hevcConfig.spsId << std::endl
       << "    ppsId: " << pConfig->encodeCodecConfig.hevcConfig.ppsId << std::endl
       << "    sliceMode: " << pConfig->encodeCodecConfig.hevcConfig.sliceMode << std::endl
       << "    sliceModeData: " << pConfig->encodeCodecConfig.hevcConfig.sliceModeData << std::endl
       << "    maxTemporalLayersMinus1: "
       << pConfig->encodeCodecConfig.hevcConfig.maxTemporalLayersMinus1 << std::endl
       << "    NV_ENC_CONFIG_HEVC_VUI_PARAMETERS:" << std::endl
       << "        overscanInfoPresentFlag: "
       << pConfig->encodeCodecConfig.hevcConfig.hevcVUIParameters.overscanInfoPresentFlag
       << std::endl
       << "        overscanInfo: "
       << pConfig->encodeCodecConfig.hevcConfig.hevcVUIParameters.overscanInfo << std::endl
       << "        videoSignalTypePresentFlag: "
       << pConfig->encodeCodecConfig.hevcConfig.hevcVUIParameters.videoSignalTypePresentFlag
       << std::endl
       << "        videoFormat: "
       << pConfig->encodeCodecConfig.hevcConfig.hevcVUIParameters.videoFormat << std::endl
       << "        videoFullRangeFlag: "
       << pConfig->encodeCodecConfig.hevcConfig.hevcVUIParameters.videoFullRangeFlag << std::endl
       << "        colourDescriptionPresentFlag: "
       << pConfig->encodeCodecConfig.hevcConfig.hevcVUIParameters.colourDescriptionPresentFlag
       << std::endl
       << "        colourPrimaries: "
       << pConfig->encodeCodecConfig.hevcConfig.hevcVUIParameters.colourPrimaries << std::endl
       << "        transferCharacteristics: "
       << pConfig->encodeCodecConfig.hevcConfig.hevcVUIParameters.transferCharacteristics
       << std::endl
       << "        colourMatrix: "
       << pConfig->encodeCodecConfig.hevcConfig.hevcVUIParameters.colourMatrix << std::endl
       << "        chromaSampleLocationFlag: "
       << pConfig->encodeCodecConfig.hevcConfig.hevcVUIParameters.chromaSampleLocationFlag
       << std::endl
       << "        chromaSampleLocationTop: "
       << pConfig->encodeCodecConfig.hevcConfig.hevcVUIParameters.chromaSampleLocationTop
       << std::endl
       << "        chromaSampleLocationBot: "
       << pConfig->encodeCodecConfig.hevcConfig.hevcVUIParameters.chromaSampleLocationBot
       << std::endl
       << "        bitstreamRestrictionFlag: "
       << pConfig->encodeCodecConfig.hevcConfig.hevcVUIParameters.bitstreamRestrictionFlag
       << std::endl
       << "    ltrTrustMode: " << pConfig->encodeCodecConfig.hevcConfig.ltrTrustMode << std::endl;
  }
  return os.str();
}
