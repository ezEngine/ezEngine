#pragma once

#include <System/Window/Window.h>

class ezFrameCaptureInterface
{
public:
  /// \brief Determine if a singleton implementing this interface has successfully been initialized and frame capture functionality is available.
  virtual bool IsInitialized() const = 0;

  /// \brief Specify the absolute base file path for storing frame captures. For the final output file name, an identifier and/or
  /// frame or capture number will be appended.
  /// Note that the final output file name is determined by the frame capture implementation. Use \ref GetLastAbsCaptureFileName()
  /// for retrieving the actual absolute file name of the most recently written capture file.
  virtual void SetAbsCaptureFilePathTemplate(const char* szFilePathTemplate) = 0;

  /// \brief Retrieve the absolute file path for storing frame captures.
  virtual const char* GetAbsCaptureFilePathTemplate() const = 0;

  /// \brief Start capturing a frame rendered to the given window.
  virtual void StartFrameCapture(ezWindowHandle hWnd) = 0;

  /// \brief Determine if a frame capture is currently in progress.
  virtual bool IsFrameCapturing() const = 0;

  /// \brief End the current frame capture and write the result to the path given by \ref SetAbsCaptureFilePathTemplate.
  virtual void EndFrameCaptureAndWriteOutput(ezWindowHandle hWnd) = 0;

  /// \brief End the current frame capture and discard the corresponding data, saving processing time and file I/O in the process.
  virtual void EndFrameCaptureAndDiscardResult(ezWindowHandle hWnd) = 0;

  /// \brief Retrieve the absolute file name of the last successful frame capture. Returns EZ_FAILURE if no successful capture has
  /// been performed.
  virtual ezResult GetLastAbsCaptureFileName(ezStringBuilder& out_sFileName) const = 0;

};

