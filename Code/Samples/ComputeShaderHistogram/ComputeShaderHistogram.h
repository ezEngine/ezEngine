#pragma once

#include <Foundation/Basics.h>

#if EZ_ENABLED(EZ_SUPPORTS_DIRECTORY_WATCHER)
#  include <Foundation/IO/DirectoryWatcher.h>
#endif
#include <Foundation/Types/UniquePtr.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <RendererCore/Material/MaterialResource.h>
#include <RendererCore/Meshes/MeshResource.h>

class ezWindow;
#if EZ_ENABLED(EZ_SUPPORTS_DIRECTORY_WATCHER)
class ezDirectoryWatcher;
#endif
/// Uses shader reloading mechanism of the ShaderExplorer sample for quick prototyping.
class ezComputeShaderHistogramApp : public ezGameApplication
{
public:
  using SUPER = ezGameApplication;

  ezComputeShaderHistogramApp();
  ~ezComputeShaderHistogramApp();

  virtual void Run() override;

  virtual void AfterCoreSystemsStartup() override;
  virtual void BeforeHighLevelSystemsShutdown() override;

private:
  void CreateHistogramQuad();
#if EZ_ENABLED(EZ_SUPPORTS_DIRECTORY_WATCHER)
  void OnFileChanged(ezStringView sFilename, ezDirectoryWatcherAction action, ezDirectoryWatcherType type);
#endif
  ezGALTextureHandle m_hScreenTexture;
  ezGALRenderTargetViewHandle m_hScreenRTV;

  // Could use buffer, but access and organisation with texture is more straight forward.
  ezGALTextureHandle m_hHistogramTexture;

  ezWindowBase* m_pWindow = nullptr;
  ezGALSwapChainHandle m_hSwapChain;

  ezShaderResourceHandle m_hRenderScreenShader;
  ezShaderResourceHandle m_hDisplayScreenShader;
  ezShaderResourceHandle m_hClearHistogramShader;
  ezShaderResourceHandle m_hComputeHistogramShader;
  ezShaderResourceHandle m_hDisplayHistogramShader;

  ezMeshBufferResourceHandle m_hHistogramQuadMeshBuffer;

#if EZ_ENABLED(EZ_SUPPORTS_DIRECTORY_WATCHER)
  ezUniquePtr<ezDirectoryWatcher> m_pDirectoryWatcher;
#endif
  bool m_bStuffChanged = false;
};
