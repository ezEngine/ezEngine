#pragma once

#include <Foundation/Basics.h>

#include <Foundation/IO/DirectoryWatcher.h>
#include <Foundation/Types/UniquePtr.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <RendererCore/Material/MaterialResource.h>
#include <RendererCore/Meshes/MeshResource.h>

class ezWindow;
class ezDirectoryWatcher;

/// Uses shader reloading mechanism of the ShaderExplorer sample for quick prototyping.
class ezComputeShaderHistogramApp : public ezGameApplication
{
public:
  typedef ezGameApplication SUPER;

  ezComputeShaderHistogramApp();
  ~ezComputeShaderHistogramApp();

  virtual ezApplication::Execution Run() override;

  virtual void AfterCoreSystemsStartup() override;
  virtual void BeforeHighLevelSystemsShutdown() override;

private:
  void CreateHistogramQuad();
  void OnFileChanged(const char* filename, ezDirectoryWatcherAction action, ezDirectoryWatcherType type);

  ezGALTextureHandle m_hScreenTexture;
  ezGALRenderTargetViewHandle m_hScreenRTV;
  ezGALResourceViewHandle m_hScreenSRV;

  // Could use buffer, but access and organisation with texture is more straight forward.
  ezGALTextureHandle m_hHistogramTexture;
  ezGALUnorderedAccessViewHandle m_hHistogramUAV;
  ezGALResourceViewHandle m_hHistogramSRV;

  ezGALSwapChainHandle m_hSwapChain;
  ezGALRenderTargetViewHandle m_hBackbufferRTV;

  ezShaderResourceHandle m_hScreenShader;
  ezShaderResourceHandle m_hHistogramDisplayShader;
  ezShaderResourceHandle m_hHistogramComputeShader;

  ezMeshBufferResourceHandle m_hHistogramQuadMeshBuffer;

  ezUniquePtr<ezDirectoryWatcher> m_directoryWatcher;

  bool m_stuffChanged;
};
