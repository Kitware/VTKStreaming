/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCPUVideoFrame.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkCPUVideoFrame
 * @brief   represents a raw video frame with data in the host (CPU) address space
 *          managed with vtkBuffer<unsigned char>
 *
 * @sa vtkRawVideoFrame, vtkOpenGLVideoFrame
 */

#ifndef vtkCPUVideoFrame_h
#define vtkCPUVideoFrame_h

#include "vtkRawVideoFrame.h"

#include "vtkBuffer.h"          // for ivar
#include "vtkNew.h"             // for vtkNew
#include "vtkStreamingCoreModule.h" // for export macro

class VTKSTREAMINGCORE_EXPORT vtkCPUVideoFrame : public vtkRawVideoFrame
{
public:
  vtkTypeMacro(vtkCPUVideoFrame, vtkRawVideoFrame);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkCPUVideoFrame* New();

  using vtkRawVideoFrame::CopyData;
  using vtkRawVideoFrame::GetData;

  ///@{
  /**
   * Parent class API for rendering. This frame does not
   * know how to render itself. See vtkOpenGLVideoFrame instead.
   */
  void Render(vtkRenderWindow* window) override;
  void Capture(vtkRenderWindow* window) override;

  ///@{
  /**
   * Parent class API for memory management.
   */
  unsigned int GetActualSize() const override;
  void AllocateDataStore() override;
  void* GetResourceHandle() noexcept override { return nullptr; }
  ///@}

  ///@{
  /**
   * Shallow copy members except the data. Calls parent class method.
   */
  void ShallowCopy(vtkRawVideoFrame* from) noexcept override;
  ///@}

protected:
  vtkCPUVideoFrame();
  ~vtkCPUVideoFrame() override;

  vtkNew<vtkBuffer<unsigned char>> Buffer;

  ///@{
  /**
   * Implement parent class Copy/Get API.
   */
  void CopyDataInternal(unsigned char* from, unsigned int size) override;
  unsigned int GetDataInternal(unsigned char*& data) const override;
  void CopyFrameDataInternal(vtkRawVideoFrame* from) override;
  ///@}

private:
  vtkCPUVideoFrame(const vtkCPUVideoFrame&) = delete;
  void operator=(const vtkCPUVideoFrame&) = delete;
};

#endif // vtkCPUVideoFrame
