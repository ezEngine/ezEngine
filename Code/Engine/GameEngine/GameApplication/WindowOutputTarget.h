#pragma once

#include <Core/GameApplication/WindowOutputTargetBase.h>
#include <GameEngine/GameEngineDLL.h>

#include <Foundation/Math/Size.h>
#include <Foundation/Types/SharedPtr.h>
#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/RendererFoundationDLL.h>
#include <RendererFoundation/Resources/ReadbackHelper.h>

struct ezGALDeviceEvent;
class ezRenderGraph;

/// \brief Creates a swapchain and keeps it up to date with the window.
///
/// If the window is resized or ezGameApplication::cvar_AppVSync changes and onSwapChainChanged is valid, the swapchain is destroyed and recreated. It is up the the application to respond to the OnSwapChainChanged callback and update any references to the swap-chain, e.g. uses in ezView or uses as render targets in ezGALRenderTargetSetup.
/// If onSwapChainChanged is not set, the swapchain will not be re-created and it is up to the application to manage the swapchain and react to window changes.
class EZ_GAMEENGINE_DLL ezWindowOutputTargetGAL : public ezWindowOutputTargetBase
{
public:
  using OnSwapChainChanged = ezDelegate<void(ezGALSwapChainHandle hSwapChain, ezSizeU32 size)>;
  ezWindowOutputTargetGAL(OnSwapChainChanged onSwapChainChanged = {});
  ~ezWindowOutputTargetGAL();

  void CreateSwapchain(const ezGALWindowSwapChainCreationDescription& desc);

  virtual void PresentImage(bool bEnableVSync) override;
  virtual void AcquireImage() override;
  virtual ezResult StartCaptureImage() override;
  virtual ezEnum<ezCaptureImageResult> WaitCaptureImage(ezImage& out_image) override;

  ezGALSwapChainHandle m_hSwapChain;

private:
  void SwapChainUpdatedEventHandler(const ezGALSwapChain* pSwapChain);
  void OnRenderEvent(const ezGALDeviceEvent& e);

  OnSwapChainChanged m_OnSwapChainChanged;
  ezSizeU32 m_Size = ezSizeU32(0, 0);
  ezGALWindowSwapChainCreationDescription m_CurrentDesc;

  // Capture image functionality
  ezGALReadbackTextureHelper m_Readback;
  ezSharedPtr<ezRenderGraph> m_pRenderGraph;
  bool m_bCaptureRequested = false;
  bool m_bCaptureInFlight = false;
  ezGALTextureCreationDescription m_CaptureBackbufferDesc; ///< Needs to be stored as the swapchain could be resized between the two capture calls.
};
