/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWEBMMuxer.cxx

  Copyright (c) 2022 Kitware, Inc
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkWEBMMuxer.h"
#include "vtkCompressedVideoPacket.h"
#include "vtkLogger.h"
#include "vtkMKVWriterImplementation.h"
#include "vtkObjectFactory.h"
#include "vtkUnsignedCharArray.h"

namespace
{
const int VideoTrackNumber = 1;
const long long NanoSecondTicks = 1000000000ll;
}

//------------------------------------------------------------------------------
struct vtkWEBMMuxer::vtkWEBMContextInternals
{
  mkvmuxer::Segment* segment;
  vtkMKVWriterImplementation* writer;
};

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkWEBMMuxer);

//------------------------------------------------------------------------------
vtkWEBMMuxer::vtkWEBMMuxer()
  : Internals(new vtkWEBMContextInternals)
{
}

//------------------------------------------------------------------------------
vtkWEBMMuxer::~vtkWEBMMuxer()
{
  if (this->IsHeaderWritten)
  {
    this->Internals->segment->Finalize();
    delete this->Internals->segment;
    delete this->Internals->writer;
    this->Internals->writer = nullptr;
    this->Internals->segment = nullptr;
  }
}

//------------------------------------------------------------------------------
void vtkWEBMMuxer::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkLogScopeFunction(TRACE);
  (void)os;
  (void)indent;
}

//------------------------------------------------------------------------------
void vtkWEBMMuxer::WriteVP8FileHeader()
{
  this->WriteFileHeader("V_VP8");
}

//------------------------------------------------------------------------------
void vtkWEBMMuxer::WriteVP9FileHeader()
{
  this->WriteFileHeader("V_VP9");
}

//------------------------------------------------------------------------------
void vtkWEBMMuxer::SetFileName(const char* filename)
{
  this->FileName = filename != nullptr ? filename : "output.webm";
}

//------------------------------------------------------------------------------
const char* vtkWEBMMuxer::GetFileName() const
{
  return this->FileName.c_str();
}

//------------------------------------------------------------------------------
void vtkWEBMMuxer::WriteFileHeader(const char* codecId)
{
  vtkLogScopeFunction(TRACE);
  if (!this->WriteToMemory)
  {
    this->Internals->writer = new vtkMKVWriterImplementation(false);
    this->Internals->writer->Open(this->FileName.c_str());
  }
  else
  {
    this->Internals->writer = new vtkMKVWriterImplementation(true);
  }
  this->Internals->segment = new mkvmuxer::Segment();
  this->Internals->segment->Init(this->Internals->writer);
  this->Internals->segment->set_mode(
    this->LiveMode ? mkvmuxer::Segment::kLive : mkvmuxer::Segment::kFile);
  this->Internals->segment->OutputCues(this->OutputCues);

  mkvmuxer::SegmentInfo* const info = this->Internals->segment->GetSegmentInfo();
  info->set_timecode_scale(this->TimeStampScale);

  std::string writerName = "vtkWEBMMuxer";
  info->set_writing_app(writerName.c_str());
  this->CodecId = codecId;

  const mkvmuxer::uint64 videoTrackId =
    this->Internals->segment->AddVideoTrack(this->Width, this->Height, ::VideoTrackNumber);
  mkvmuxer::VideoTrack* const videoTrack =
    static_cast<mkvmuxer::VideoTrack*>(this->Internals->segment->GetTrackByNumber(videoTrackId));
  videoTrack->SetStereoMode(mkvmuxer::VideoTrack::StereoMode::kMono);
  videoTrack->set_codec_id(codecId);
  videoTrack->set_display_width(this->Width);
  videoTrack->set_display_height(this->Height);
  this->IsHeaderWritten = true;
}

//------------------------------------------------------------------------------
void vtkWEBMMuxer::WriteWebmBlock(vtkCompressedVideoPacket* packet)
{
  vtkLogScopeFunction(TRACE);

  if (this->Width != packet->GetWidth() || this->Height != packet->GetHeight())
  {
    if (this->WriteToMemory)
    {
      // close this stream.
      this->WriteFileTrailer();
      // set the new width, height.
      this->SetWidth(packet->GetWidth());
      this->SetHeight(packet->GetHeight());
      // start a new one.
      this->WriteFileHeader(this->CodecId.c_str());
    }
    else
    {
      vtkLog(ERROR, << "Dynamic display dimensions are only supported when writing to memory.")
    }
  }

  mkvmuxer::Segment* const segment = reinterpret_cast<mkvmuxer::Segment*>(this->Internals->segment);
  mkvmuxer::int64 timecode = packet->GetPresentationTS();
  vtkLogF(INFO, "webm-block-timecode: %lld", timecode);
  if (this->ForceNewClusters)
  {
    segment->ForceNewClusterOnNextFrame();
  }
  unsigned char* buffer = nullptr;
  int size = packet->GetData(buffer);
  bool isKeyFrame = packet->GetIsKeyFrame();
  segment->AddFrame(buffer, size, ::VideoTrackNumber, timecode, isKeyFrame);
}

//------------------------------------------------------------------------------
void vtkWEBMMuxer::WriteFileTrailer()
{
  vtkLogScopeFunction(TRACE);
  if (!this->IsHeaderWritten)
  {
    return;
  }
  this->Internals->segment->Finalize();
  delete this->Internals->segment;
  delete this->Internals->writer;
  this->Internals->writer = nullptr;
  this->Internals->segment = nullptr;
  this->IsHeaderWritten = false;
}

//------------------------------------------------------------------------------
void vtkWEBMMuxer::Flush()
{
  vtkLogScopeFunction(TRACE);
  if (this->Internals->writer != nullptr)
  {
    this->Internals->writer->GetDynamicBuffer()->Reset();
  }
}

//------------------------------------------------------------------------------
vtkUnsignedCharArray* vtkWEBMMuxer::GetDynamicBuffer()
{
  vtkLogScopeFunction(TRACE);
  return this->Internals->writer->GetDynamicBuffer();
}
