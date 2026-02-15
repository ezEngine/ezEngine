#pragma once

#ifdef BUILDSYSTEM_ENGINE_PROCESS_SHARED_TEXTURE

#  include <Core/System/Window.h>
#  include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>
#  include <Foundation/Containers/HybridArray.h>
#  include <Foundation/Math/Size.h>
#  include <Foundation/Threading/AtomicInteger.h>
#  include <Foundation/Strings/String.h>
#  include <Foundation/Time/Time.h>
#  include <Foundation/Types/Types.h>
#  include <RendererFoundation/Device/Device.h>
#  include <RendererFoundation/Device/SwapChain.h>
#  include <RendererFoundation/Resources/ReadbackHelper.h>
#  include <RendererFoundation/Resources/Texture.h>
#  include <RendererCore/Shader/ShaderResource.h>

/// \brief Represents the window inside the editor process, into which the engine process renders.
class ezEngineViewWindow : public ezWindowBase
{
public:
  explicit ezEngineViewWindow(ezGALDevice* pDevice);
  ~ezEngineViewWindow() override;

  ezResult UpdateWindow(ezWindowHandle hParentWindow, ezUInt16 uiWidth, ezUInt16 uiHeight);

  bool IsOpenMessagePending() const { return m_bOpenMessagePending; }

  void RequestScreenshot(ezStringView sOutputFile);

  ezResult FillOpenMessage(ezViewOpenSharedTexturesMsgToEngine& msg);
  ezResult FillRedrawMessage(ezViewRedrawMsgToEngine& msg);

  void TryCompleteScreenshotReadback();
  void Render(ezUInt32 uiCurrentTextureIndex, ezUInt64 uiCurrentSemaphoreValue);

  ezSizeU32 GetClientAreaSize() const override;
  ezWindowHandle GetNativeWindowHandle() const override;
  void ProcessWindowMessages() override;
  bool IsFullscreenWindow(bool bOnlyProperFullscreenMode = false) const override;
  bool IsVisible() const override;
  void AddReference() override;
  void RemoveReference() override;

private:
  static constexpr ezUInt32 s_SharedTextureCount = 3;

  void CreateSharedTextures(ezUInt16 uiWidth, ezUInt16 uiHeight);
  void DestroySharedTextures();
  bool HasInFlightTextures() const;
  void ClearTimedOutInFlightTextures(const ezTime& now);
  void ResetInFlightTexture(ezUInt32 uiIndex);

  ezWindowHandle m_hWnd;
  ezAtomicInteger32 m_iReferenceCount = 0;
  ezGALDevice* m_pDevice = nullptr;
  ezGALSwapChainHandle m_hSwapChain;

  ezUInt16 m_uiWidth = 0;
  ezUInt16 m_uiHeight = 0;
  bool m_bResizePending = false;
  ezUInt16 m_uiPendingWidth = 0;
  ezUInt16 m_uiPendingHeight = 0;
  bool m_bSwapChainResizePending = false;
  bool m_bReopenSharedTextures = false;

  ezGALTextureCreationDescription m_SharedTextureDesc;
  ezGALTextureHandle m_hSharedTextures[s_SharedTextureCount] = {};
  ezUInt64 m_uiSharedTextureSemaphoreCount[s_SharedTextureCount] = {};
  bool m_bSharedTextureInUse[s_SharedTextureCount] = {};
  bool m_bInFlightTimedOut[s_SharedTextureCount] = {};
  ezTime m_TextureInFlightStart[s_SharedTextureCount] = {};
  ezUInt32 m_uiCurrentSharedTextureIndex = 0;
  ezUInt32 m_uiTextureGeneration = 0;
  bool m_bOpenMessagePending = false;
  ezString m_sPendingScreenshotFile;
  bool m_bReadbackInFlight = false;
  ezUInt32 m_uiReadbackTextureIndex = 0;
  ezGALReadbackTextureHelper m_Readback;
  ezShaderResourceHandle m_hCopyShader;
};

#endif
