/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWEBMWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkWEBMWriter.h"
#include "vtkCodedVideoPacket.h"
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
struct vtkWEBMWriter::vtkWEBMContextInternals
{
  mkvmuxer::Segment* segment;
  vtkMKVWriterImplementation* writer;
  int64_t last_pts_ns = 0;
};

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkWEBMWriter);

//------------------------------------------------------------------------------
vtkWEBMWriter::vtkWEBMWriter()
  : Internals(new vtkWEBMContextInternals)
{
}

//------------------------------------------------------------------------------
vtkWEBMWriter::~vtkWEBMWriter()
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
void vtkWEBMWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkLogScopeFunction(TRACE);
  (void)os;
  (void)indent;
}

//------------------------------------------------------------------------------
void vtkWEBMWriter::WriteVP8FileHeader()
{
  this->WriteFileHeader("V_VP8");
}

//------------------------------------------------------------------------------
void vtkWEBMWriter::WriteVP9FileHeader()
{
  this->WriteFileHeader("V_VP9");
}

//------------------------------------------------------------------------------
void vtkWEBMWriter::SetFileName(const char* filename)
{
  this->FileName = filename != nullptr ? filename : "output.webm";
}

//------------------------------------------------------------------------------
const char* vtkWEBMWriter::GetFileName() const
{
  return this->FileName.c_str();
}

//------------------------------------------------------------------------------
void vtkWEBMWriter::WriteFileHeader(const char* codecId)
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

  std::string writerName = "vtkWEBMWriter";
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
void vtkWEBMWriter::WriteWebmBlock(vtkCodedVideoPacket* packet)
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
  const auto frameRateInv = static_cast<double>(1) / this->Framerate;
  const mkvmuxer::int64 unit_frame_interval = ::NanoSecondTicks * frameRateInv;
  mkvmuxer::int64 pts_ns = packet->GetPresentationTS() * unit_frame_interval;

  vtkLog(TRACE, << "Input pts_ns " << pts_ns);
  if (pts_ns <= this->Internals->last_pts_ns)
  {
    pts_ns = this->Internals->last_pts_ns + ::NanoSecondTicks * frameRateInv;
  }
  vtkLog(TRACE, << "Final pts_ns " << pts_ns);

  this->Internals->last_pts_ns = pts_ns;
  if (this->ForceNewClusters)
  {
    segment->ForceNewClusterOnNextFrame();
  }
  unsigned char* buffer = nullptr;
  int size = packet->GetData(buffer);
  bool isKeyFrame = packet->GetIsKeyFrame();
  segment->AddFrame(buffer, size, ::VideoTrackNumber, pts_ns, isKeyFrame);
}

//------------------------------------------------------------------------------
void vtkWEBMWriter::WriteFileTrailer()
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
void vtkWEBMWriter::Flush()
{
  vtkLogScopeFunction(TRACE);
  if (this->Internals->writer != nullptr)
  {
    this->Internals->writer->GetDynamicBuffer()->Reset();
  }
}

//------------------------------------------------------------------------------
vtkUnsignedCharArray* vtkWEBMWriter::GetDynamicBuffer()
{
  vtkLogScopeFunction(TRACE);
  return this->Internals->writer->GetDynamicBuffer();
}
