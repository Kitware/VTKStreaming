/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWEBMMuxer.h

  Copyright (c) 2022 Kitware, Inc
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkWEBMMuxer
 * @brief   class for muxing VP8, VP9 compressed packets into WEBM container.
 *
 * vtkWEBMMuxer is used to save VP8, VP9 encoded video packets to memory using
 * webm multimedia container format. The webm format is a trimmed down mkv that
 * supports VP9 and VP8 bitstreams.
 *
 * You can write headers, trailers and webm blocks. This class is capable of writing
 * into a dynamic buffer rather than a file.
 *
 * Be sure to fetch the dynamic buffer and flush its contents periodically
 * by calling vtkWEBMMuxer::Flush() to avoid out of memory errors.
 *
 * @sa
 * vtkCompressedVideoPacket
 */

#ifndef vtkWEBMMuxer_h
#define vtkWEBMMuxer_h

#include "vtkObject.h"
#include "vtkStreamingWEBMModule.h" // for export macro

#include <memory> // for ivar
#include <string> // for ivar

class vtkCompressedVideoPacket;
class vtkUnsignedCharArray;
class vtkWEBMContextInternals;

class VTKSTREAMINGWEBM_EXPORT vtkWEBMMuxer : public vtkObject
{
public:
  vtkTypeMacro(vtkWEBMMuxer, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkWEBMMuxer* New();

  ///@{
  /**
   * Check if the writer has already written a header.
   */
  vtkGetMacro(IsHeaderWritten, bool);
  ///@}

  ///@{
  /**
   * Force new clusters for every frame. Generally, the mkv container holds a number of blocks in a
   * single cluster. Enable this setting if you wish to save each frame as an independent cluster.
   */
  vtkSetMacro(ForceNewClusters, bool);
  vtkGetMacro(ForceNewClusters, bool);
  ///@}

  ///@{
  /**
   * Enable live mode. When true, the EBML segment size bit is set to 1 to indicate an ever-growing
   * sequence of clusters.
   */
  vtkSetMacro(LiveMode, bool);
  vtkGetMacro(LiveMode, bool);
  ///@}

  ///@{
  /**
   * Write cues. Cues are used to improve seeking in video playback.
   * Note: it's recommended to disable this setting when LiveMode is true and seeking is not
   * desired. Ex:- live streams
   */
  vtkSetMacro(OutputCues, bool);
  vtkGetMacro(OutputCues, bool);
  ///@}

  ///@{
  /**
   * Write to a dynamic buffer instead of a file on disk.
   * You can retrieve the dynamic buffer with vtkWEBMMuxer::GetDynamicBuffer()
   */
  vtkSetMacro(WriteToMemory, bool);
  vtkGetMacro(WriteToMemory, bool);
  ///@}

  ///@{
  /**
   * Control the scaling of the timestamp of the output video stream.
   * Note: setting has no effect if ForceNewClusters is true because every cluster ends up
   * with a single block.
   */
  vtkSetMacro(TimeStampScale, long int);
  vtkGetMacro(TimeStampScale, long int);
  ///@}

  ///@{
  /**
   * Control the framerate of the output video stream.
   */
  vtkSetMacro(Framerate, unsigned int);
  vtkGetMacro(Framerate, unsigned int);
  ///@}

  ///@{
  /**
   * Set/Get width of the frame.
   */
  vtkSetMacro(Width, unsigned int);
  vtkGetMacro(Width, unsigned int);
  ///@}

  ///@{
  /**
   * Set/Get height of the frame.
   */
  vtkSetMacro(Height, unsigned int);
  vtkGetMacro(Height, unsigned int);
  ///@}

  ///@{
  /**
   * Set/Get file name.
   */
  void SetFileName(const char* filename);
  const char* GetFileName() const;
  ///@}

  ///@{
  /**
   * Writing API
   */
  void WriteVP9FileHeader();
  void WriteVP8FileHeader();
  void WriteWebmBlock(vtkCompressedVideoPacket* packet);
  void WriteFileTrailer();
  void Flush();
  ///@}

  /**
   * Get the dynamic buffer. It is used when WriteToMemory is true.
   */
  vtkUnsignedCharArray* GetDynamicBuffer();

protected:
  vtkWEBMMuxer();
  ~vtkWEBMMuxer() override;

  bool IsHeaderWritten = false;
  bool ForceNewClusters = false;
  bool LiveMode = false;
  bool OutputCues = true;
  bool WriteToMemory = false;
  long int TimeStampScale = 100;
  unsigned int Framerate = 30;
  unsigned int Width = 0;
  unsigned int Height = 0;
  std::string FileName, CodecId;

  void WriteFileHeader(const char* codecId);

private:
  vtkWEBMMuxer(const vtkWEBMMuxer&) = delete;
  void operator=(const vtkWEBMMuxer&) = delete;

  struct vtkWEBMContextInternals;
  std::unique_ptr<vtkWEBMContextInternals> Internals;
};

#endif // vtkWEBMMuxer_h
