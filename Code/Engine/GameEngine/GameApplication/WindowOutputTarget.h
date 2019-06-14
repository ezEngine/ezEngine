#pragma once

#include <GameEngine/GameEngineDLL.h>
#include <GameEngine/GameApplication/WindowOutputTargetBase.h>

#include <RendererFoundation/RendererFoundationDLL.h>

struct ezGALSwapChainCreationDescription;

class EZ_GAMEENGINE_DLL ezWindowOutputTargetGAL : public ezWindowOutputTargetBase
{
public:
  ezWindowOutputTargetGAL();
  ~ezWindowOutputTargetGAL();

  void CreateSwapchain(const ezGALSwapChainCreationDescription& desc);

  virtual void Present(bool bEnableVSync) override;
  virtual ezResult CaptureImage(ezImage& out_Image) override;

  ezGALSwapChainHandle m_hSwapChain;
};




