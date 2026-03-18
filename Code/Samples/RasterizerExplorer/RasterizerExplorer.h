#pragma once

#include <Foundation/Application/Application.h>
#include <Foundation/Types/UniquePtr.h>
#include <RendererFoundation/RendererFoundationDLL.h>

class ezWindow;
class ezCamera;
class ezGALDevice;
class ezRasterizerView;
class ezRasterizerObject;

class ezRasterizerExplorerApp : public ezApplication
{
public:
  using SUPER = ezApplication;

  ezRasterizerExplorerApp();

  virtual void Run() override;
  virtual void AfterCoreSystemsStartup() override;
  virtual void BeforeHighLevelSystemsShutdown() override;

private:
  void UpdateSwapChain();
  void PrintTestCode();
  void RenderRasterizer();

  ezWindow* m_pWindow = nullptr;
  ezGALDevice* m_pDevice = nullptr;

  ezGALSwapChainHandle m_hSwapChain;
  ezGALTextureHandle m_hDepthStencilTexture;
  ezGALTextureHandle m_hRasterizerTexture;

  ezUniquePtr<ezCamera> m_pCamera;
  ezUniquePtr<ezRasterizerView> m_pRasterizerView;

  static constexpr ezUInt32 RasterizerSize = 256;
};
