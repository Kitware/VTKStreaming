/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVideoEncoderFactory.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkVideoEncoderFactory.h"
#include "vtkFFmpegHardwareEncoder.h"
#include "vtkFFmpegSoftwareEncoder.h"
#include "vtkJPEGVideoEncoder.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkVideoCodecTypes.h"

#include <map>
#include <set>
#include <string>
#include <vector>

namespace
{

using EncoderTypeEnum = vtkVideoEncoderFactory::EncoderTypeEnum;
std::string EncoderTypeEnum2Str(::EncoderTypeEnum encoderType)
{
  switch (encoderType)
  {
    case ::EncoderTypeEnum::AMD:
      return "AMD";
    case ::EncoderTypeEnum::NVIDIA:
      return "NVIDIA";
    case ::EncoderTypeEnum::Intel:
      return "Intel";
    case ::EncoderTypeEnum::Software:
      return "Software";
    case ::EncoderTypeEnum::VideoToolbox:
      return "VideoToolbox";
    case ::EncoderTypeEnum::None:
    default:
      return "None";
  }
}

std::string EncoderTypeEnum2Str(int pref)
{
  if (pref >= -1 && pref <= ::EncoderTypeEnum::MaxTypes)
  {
    return ::EncoderTypeEnum2Str(static_cast<::EncoderTypeEnum>(pref));
  }
  return "None";
}

std::string CodecTypeEnum2Str(int codec)
{
  if (codec >= 0 && codec <= static_cast<int>(VTKVideoCodecType::VTKVC_MaxNumberOfSupportedCodecs))
  {
    return vtkVideoCodecTypeUtilities::ToString(static_cast<VTKVideoCodecType>(codec));
  }
  return "None";
}

bool Initialized = false;
std::map<::EncoderTypeEnum, std::set<VTKVideoCodecType>> AvailableEncoders;

}

class vtkVideoEncoderFactory::vtkInternals
{
public:
  ::EncoderTypeEnum PreferredEncoderType = ::EncoderTypeEnum::None;
  vtkNew<vtkFFmpegHardwareEncoder> HWEncoder;
  vtkNew<vtkFFmpegSoftwareEncoder> SWEncoder;
};

vtkStandardNewMacro(vtkVideoEncoderFactory);

//----------------------------------------------------------------------------
vtkVideoEncoderFactory::vtkVideoEncoderFactory()
  : Internals(std::unique_ptr<vtkInternals>(new vtkInternals()))
{
}

//----------------------------------------------------------------------------
vtkVideoEncoderFactory::~vtkVideoEncoderFactory() = default;

//----------------------------------------------------------------------------
void vtkVideoEncoderFactory::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  vtkVideoEncoderFactory::LogAvailableEncoders();

  const auto& internals = (*this->Internals);
  os << "PreferredEncoderType : " << ::EncoderTypeEnum2Str(internals.PreferredEncoderType) << "\n";
}

//----------------------------------------------------------------------------
void vtkVideoEncoderFactory::Initialize()
{
  vtkLogScopeFunction(TRACE);
  if (::Initialized)
  {
    return;
  }
  vtkLog(INFO, << "Initializing encoder availability matrix.");
  auto& table = ::AvailableEncoders;
  for (int encoderType = 0; encoderType < ::EncoderTypeEnum::MaxTypes; ++encoderType)
  {
    for (int codec = 0;
         codec < static_cast<int>(VTKVideoCodecType::VTKVC_MaxNumberOfSupportedCodecs); ++codec)
    {
      const auto codecEnum = static_cast<VTKVideoCodecType>(codec);
      const auto encoderTypeEnum = static_cast<::EncoderTypeEnum>(encoderType);
      if (!vtkVideoEncoderFactory::SupportsEncoderTypeWithCodec(encoderTypeEnum, codecEnum))
      {
        continue;
      }
      table[encoderTypeEnum].insert(codecEnum);
    }
  }
  ::Initialized = true;
}

//----------------------------------------------------------------------------
void vtkVideoEncoderFactory::LogAvailableEncoders()
{
  vtkLogScopeFunction(TRACE);
  if (!::Initialized)
  {
    this->Initialize();
  }
  vtkLog(INFO, << "Available encoders - ");
  for (const auto& encoder : ::AvailableEncoders)
  {
    const auto& encoderType = encoder.first;
    const auto& codecs = encoder.second;
    vtkLog(INFO, << ::EncoderTypeEnum2Str(encoderType));
    for (const auto& codecType : codecs)
    {
      vtkLog(INFO, << "|--" << vtkVideoCodecTypeUtilities::ToString(codecType));
    }
  }
}

//----------------------------------------------------------------------------
void vtkVideoEncoderFactory::SetEncoderPreference(int encoderType)
{
  auto& internals = (*this->Internals);
  if (encoderType >= -1 && encoderType < EncoderTypeEnum::MaxTypes)
  {
    internals.PreferredEncoderType = static_cast<EncoderTypeEnum>(encoderType);
  }
  else
  {
    internals.PreferredEncoderType = EncoderTypeEnum::None;
  }
}

//----------------------------------------------------------------------------
void vtkVideoEncoderFactory::SetEncoderPreference(EncoderTypeEnum encoderType)
{
  auto& internals = (*this->Internals);
  internals.PreferredEncoderType = encoderType;
}

//----------------------------------------------------------------------------
void vtkVideoEncoderFactory::ClearEncoderPreference()
{
  auto& internals = (*this->Internals);
  internals.PreferredEncoderType = ::EncoderTypeEnum::None;
}

//----------------------------------------------------------------------------
void vtkVideoEncoderFactory::PreferAMD()
{
  auto& internals = (*this->Internals);
  internals.PreferredEncoderType = ::EncoderTypeEnum::AMD;
}

//----------------------------------------------------------------------------
void vtkVideoEncoderFactory::PreferIntel()
{

  auto& internals = (*this->Internals);
  internals.PreferredEncoderType = ::EncoderTypeEnum::Intel;
}

//----------------------------------------------------------------------------
void vtkVideoEncoderFactory::PreferNVIDIA()
{
  auto& internals = (*this->Internals);
  internals.PreferredEncoderType = ::EncoderTypeEnum::NVIDIA;
}

//----------------------------------------------------------------------------
void vtkVideoEncoderFactory::PreferSoftware()
{
  auto& internals = (*this->Internals);
  internals.PreferredEncoderType = ::EncoderTypeEnum::Software;
}

//----------------------------------------------------------------------------
void vtkVideoEncoderFactory::PreferVideoToolbox()
{
  auto& internals = (*this->Internals);
  internals.PreferredEncoderType = ::EncoderTypeEnum::VideoToolbox;
}

//----------------------------------------------------------------------------
vtkVideoEncoder* vtkVideoEncoderFactory::NewEncoder()
{
  vtkLogScopeFunction(TRACE);
  if (!::Initialized)
  {
    vtkVideoEncoderFactory::Initialize();
    this->LogAvailableEncoders();
  }
  auto& internals = (*this->Internals);
  if (::AvailableEncoders[internals.PreferredEncoderType].empty())
  {
    // try other encoder types.
    for (const auto& encoders : ::AvailableEncoders)
    {
      const auto& encoderType = encoders.first;
      const auto& codecs = encoders.second;
      // we'll take the first codec the encoder supports.
      if (!codecs.empty())
      {
        return this->CreateEncoder(encoderType);
      }
    }
  }
  else
  {
    return this->CreateEncoder(internals.PreferredEncoderType);
  }

  vtkLog(ERROR, << "Unable to find any encoder on this system.");
  return nullptr;
}

//----------------------------------------------------------------------------
vtkVideoEncoder* vtkVideoEncoderFactory::NewEncoder(VTKVideoCodecType codec)
{
  vtkLogScopeFunction(TRACE);
  if (!::Initialized)
  {
    vtkVideoEncoderFactory::Initialize();
    this->LogAvailableEncoders();
  }
  auto& internals = (*this->Internals);

  if (::AvailableEncoders[internals.PreferredEncoderType].count(codec))
  {
    // yay
    return this->CreateEncoderWithCodec(internals.PreferredEncoderType, codec);
  }
  else
  {
    // try other encoder types for the requested codec.
    for (const auto& encoders : ::AvailableEncoders)
    {
      const auto& encoderType = encoders.first;
      const auto& codecs = encoders.second;
      if (codecs.count(codec))
      {
        return this->CreateEncoderWithCodec(encoderType, codec);
      }
    }
  }

  vtkLog(ERROR, << "Unable to find a suitable encoder on this system. Requested codec type - "
                << vtkVideoCodecTypeUtilities::ToString(codec));
  return nullptr;
}

//----------------------------------------------------------------------------
vtkVideoEncoder* vtkVideoEncoderFactory::NewHardwareEncoder(VTKVideoCodecType codec)
{
  vtkLogScopeFunction(TRACE);
  if (!::Initialized)
  {
    vtkVideoEncoderFactory::Initialize();
    this->LogAvailableEncoders();
  }
  auto& internals = (*this->Internals);
  vtkVideoEncoder* result = nullptr;

  std::vector<::EncoderTypeEnum> hardwareEncoderTypes = { ::EncoderTypeEnum::NVIDIA,
    ::EncoderTypeEnum::AMD, ::EncoderTypeEnum::Intel, ::EncoderTypeEnum::VideoToolbox };

  for (int i = 0; i < hardwareEncoderTypes.size() && result == nullptr; ++i)
  {
    result = this->CreateEncoderWithCodec(hardwareEncoderTypes[i], codec);
  }

  if (result == nullptr)
  {
    vtkLog(
      ERROR, << "Unable to find a hardware accelerated encoder on this system. Requested codec - "
             << vtkVideoCodecTypeUtilities::ToString(codec));
  }

  return result;
}

//----------------------------------------------------------------------------
vtkVideoEncoder* vtkVideoEncoderFactory::NewSoftwareEncoder(VTKVideoCodecType codec)
{
  vtkLogScopeFunction(TRACE);
  if (!::Initialized)
  {
    vtkVideoEncoderFactory::Initialize();
    this->LogAvailableEncoders();
  }
  auto& internals = (*this->Internals);
  vtkVideoEncoder* result = this->CreateEncoderWithCodec(EncoderTypeEnum::Software, codec);
  if (result == nullptr)
  {
    vtkLog(ERROR, << "Unable to find a software encoder on this system. Requested codec - "
                  << vtkVideoCodecTypeUtilities::ToString(codec));
  }
  return result;
}

//----------------------------------------------------------------------------
bool vtkVideoEncoderFactory::SupportsEncoderTypeWithCodec(
  EncoderTypeEnum encoderType, VTKVideoCodecType codec)
{
  vtkLogScopeFunction(TRACE);
  vtkLogScopeF(TRACE, "Readiness check %s %s", ::EncoderTypeEnum2Str(encoderType).c_str(),
    vtkVideoCodecTypeUtilities::ToString(codec));
  bool success = false;
  switch (encoderType)
  {
    case ::EncoderTypeEnum::AMD:
    {
      this->Internals->HWEncoder->PreferAMDEncoders();
      this->Internals->HWEncoder->SetCodec(codec);
      success = this->Internals->HWEncoder->Initialize();
      this->Internals->HWEncoder->Shutdown();
      break;
    }
    case ::EncoderTypeEnum::Intel:
    {
      this->Internals->HWEncoder->PreferIntelEncoders();
      this->Internals->HWEncoder->SetCodec(codec);
      success = this->Internals->HWEncoder->Initialize();
      this->Internals->HWEncoder->Shutdown();
      break;
    }
    case ::EncoderTypeEnum::NVIDIA:
    {
      this->Internals->HWEncoder->PreferNVIDIAEncoders();
      this->Internals->HWEncoder->SetCodec(codec);
      success = this->Internals->HWEncoder->Initialize();
      this->Internals->HWEncoder->Shutdown();
      break;
    }
    case ::EncoderTypeEnum::VideoToolbox:
    {
#if __APPLE__
      this->Internals->HWEncoder->ClearGPUPreference();
      this->Internals->HWEncoder->SetCodec(codec);
      success = this->Internals->HWEncoder->Initialize();
#endif
      break;
    }
    case ::EncoderTypeEnum::Software:
    default: // none fallback to software.
    {
      if (codec == VTKVideoCodecType::VTKVC_JPEG)
      {
        success = true;
        break;
      }
      this->Internals->HWEncoder->ClearGPUPreference();
      this->Internals->SWEncoder->SetCodec(codec);
      success = this->Internals->SWEncoder->Initialize();
      this->Internals->SWEncoder->Shutdown();
      break;
    }
  }

  return success;
}

//----------------------------------------------------------------------------
bool vtkVideoEncoderFactory::SupportsEncoderType(EncoderTypeEnum encoderType)
{
  vtkLogScopeFunction(TRACE);
  vtkLog(TRACE, << "Trying video encoder type - " << ::EncoderTypeEnum2Str(encoderType) << "...");

  bool success = false;
  for (int i = 0;
       i < static_cast<int>(VTKVideoCodecType::VTKVC_MaxNumberOfSupportedCodecs) && !success; ++i)
  {
    success = vtkVideoEncoderFactory::SupportsEncoderTypeWithCodec(
      encoderType, static_cast<VTKVideoCodecType>(i));
  }

  return success;
}

//----------------------------------------------------------------------------
vtkVideoEncoder* vtkVideoEncoderFactory::CreateEncoderWithCodec(
  EncoderTypeEnum encoderType, VTKVideoCodecType codec)
{
  vtkLogScopeFunction(TRACE);
  if (!::AvailableEncoders[encoderType].count(codec))
  {
    return nullptr;
  }

  vtkVideoEncoder* preferredEncoder = nullptr;
  switch (encoderType)
  {
    case ::EncoderTypeEnum::AMD:
    {
      auto enc = vtkFFmpegHardwareEncoder::New();
      enc->PreferAMDEncoders();
      enc->SetCodec(codec);
      preferredEncoder = enc;
      break;
    }
    case ::EncoderTypeEnum::Intel:
    {
      auto enc = vtkFFmpegHardwareEncoder::New();
      enc->PreferIntelEncoders();
      enc->SetCodec(codec);
      preferredEncoder = enc;
      break;
    }
    case ::EncoderTypeEnum::NVIDIA:
    {
      auto enc = vtkFFmpegHardwareEncoder::New();
      enc->PreferNVIDIAEncoders();
      enc->SetCodec(codec);
      preferredEncoder = enc;
      break;
    }
    case ::EncoderTypeEnum::VideoToolbox:
    {
#if __APPLE__
      auto enc = vtkFFmpegHardwareEncoder::New();
      enc->SetCodec(codec);
      preferredEncoder = enc;
#endif
      break;
    }
    case ::EncoderTypeEnum::Software:
    default: // none fallback to software.
    {
      if (codec == VTKVideoCodecType::VTKVC_JPEG)
      {
        auto enc = vtkJPEGVideoEncoder::New();
        enc->SetCodec(codec);
        preferredEncoder = enc;
      }
      else
      {
        auto enc = vtkFFmpegSoftwareEncoder::New();
        enc->SetCodec(codec);
        preferredEncoder = enc;
      }
      break;
    }
  }

  return preferredEncoder;
}

//----------------------------------------------------------------------------
vtkVideoEncoder* vtkVideoEncoderFactory::CreateEncoder(EncoderTypeEnum encoderType)
{
  vtkLogScopeFunction(TRACE);
  vtkLog(TRACE, << "Trying video encoder type - " << ::EncoderTypeEnum2Str(encoderType) << "...");
  auto& internals = (*this->Internals);

  vtkVideoEncoder* preferredEncoder = nullptr;
  for (int i = 0; i < static_cast<int>(VTKVideoCodecType::VTKVC_MaxNumberOfSupportedCodecs) &&
       preferredEncoder == nullptr;
       ++i)
  {
    preferredEncoder = this->CreateEncoderWithCodec(encoderType, static_cast<VTKVideoCodecType>(i));
  }

  return preferredEncoder;
}
