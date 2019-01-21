#pragma once

#include <RenderDocPlugin/Basics.h>
#include <GameEngine/Interfaces/FrameCaptureInterface.h>
#include <Foundation/Configuration/Plugin.h>
#include <Foundation/Configuration/Singleton.h>

struct RENDERDOC_API_1_4_0;

/// \brief RenderDoc implementation of the ezFrameCaptureInterface interface
///
/// Adds support for capturing frames through RenderDoc.
/// When the plugin gets loaded, an ezRenderDoc instance is created and initialized.
/// It tries to find a RenderDoc DLL dynamically, so for initialization to succeed,
/// the DLL has to be available in some search directory (e.g. binary folder or PATH).
/// If an outdated RenderDoc DLL is found, initialization will fail and the plugin will be deactivated.
///
/// For interface documentation see \ref ezFrameCaptureInterface
class EZ_RENDERDOCPLUGIN_DLL ezRenderDoc : public ezFrameCaptureInterface
{
  EZ_DECLARE_SINGLETON_OF_INTERFACE(ezRenderDoc, ezFrameCaptureInterface);

public:
  ezRenderDoc();
  virtual ~ezRenderDoc();

  virtual bool IsInitialized() const override;
  virtual void SetAbsCaptureFilePathTemplate(const char* szFilePathTemplate) override;
  virtual const char* GetAbsCaptureFilePathTemplate() const override;
  virtual void StartFrameCapture(ezWindowHandle hWnd) override;
  virtual bool IsFrameCapturing() const override;
  virtual void EndFrameCaptureAndWriteOutput(ezWindowHandle hWnd) override;
  virtual void EndFrameCaptureAndDiscardResult(ezWindowHandle hWnd) override;
  virtual ezResult GetLastAbsCaptureFileName(ezStringBuilder& out_sFileName) const override;

private:

  RENDERDOC_API_1_4_0* m_pRenderDocAPI = nullptr;
  HMODULE m_HandleToFree = nullptr;

};

EZ_DYNAMIC_PLUGIN_DECLARATION(EZ_RENDERDOCPLUGIN_DLL, ezRenderDocPlugin);
