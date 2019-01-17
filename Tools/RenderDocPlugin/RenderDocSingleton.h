#pragma once

#include <RenderDocPlugin/Basics.h>
#include <GameEngine/Interfaces/FrameCaptureInterface.h>
#include <Foundation/Configuration/Plugin.h>
#include <Foundation/Configuration/Singleton.h>

struct RENDERDOC_API_1_4_0;

class EZ_RENDERDOCPLUGIN_DLL ezRenderDoc : public ezFrameCaptureInterface
{
  EZ_DECLARE_SINGLETON_OF_INTERFACE(ezRenderDoc, ezFrameCaptureInterface);

public:
  ezRenderDoc();
  virtual ~ezRenderDoc();

  /// \brief Determine if the RenderDoc plugin has successfully been loaded and frame capture functionality is available.
  virtual bool IsInitialized() const override;

  /// \brief Specify the absolute base file path for storing frame captures. For the final output file name, an identifier and/or
  /// frame or capture number will be appended.
  /// Note that the final output file name is determined by the frame capture implementation. Use \ref GetLastCaptureFileName
  /// for retrieving the actual absolute file name of the most recently written capture file.
  virtual void SetAbsCaptureFilePathTemplate(const char* szFilePathTemplate) override;

  /// \brief Retrieve the absolute file path for storing frame captures.
  virtual const char* GetAbsCaptureFilePathTemplate() const override;

  /// \brief Start capturing a frame rendered to the given window.
  virtual void StartFrameCapture(ezWindowHandle hWnd) override;

  /// \brief Determine if a frame capture is currently in progress.
  virtual bool IsFrameCapturing() const override;

  /// \brief End the current frame capture and write the result to the path given by \ref SetAbsCaptureFilePathTemplate.
  virtual void EndFrameCaptureAndWriteOutput(ezWindowHandle hWnd) override;

  /// \brief End the current frame capture and discard the corresponding data, saving processing time and file I/O in the process.
  virtual void EndFrameCaptureAndDiscardResult(ezWindowHandle hWnd) override;

  /// \brief Retrieve the absolute file name of the last successful frame capture. Returns EZ_FAILURE if no successful capture has
  /// been performed.
  virtual ezResult GetLastAbsCaptureFileName(ezStringBuilder& out_sFileName) const override;

private:

  RENDERDOC_API_1_4_0* m_pRenderDocAPI = nullptr;
  HMODULE m_HandleToFree = nullptr;

};

EZ_DYNAMIC_PLUGIN_DECLARATION(EZ_RENDERDOCPLUGIN_DLL, ezRenderDocPlugin);
