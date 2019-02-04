#pragma once

#include <GameEngine/Basics.h>
#include <GameEngine/GameApplication/WindowOutputTargetBase.h>

#include <RendererFoundation/Basics.h>

class ezWindowOutputTargetGAL : public ezWindowOutputTargetBase
{
public:
  virtual void Present(bool bEnableVSync) override;
  virtual ezResult CaptureImage(ezImage& out_Image) override;

  ezGALSwapChainHandle m_hSwapChain;
};

