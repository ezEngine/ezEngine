#pragma once

#include <GameEngine/GameApplication/GameApplication.h>
#include <Foundation/Types/UniquePtr.h>
#include <RendererCore/Meshes/MeshResource.h>
#include <RendererCore/Material/MaterialResource.h>
#include <Foundation/IO/DirectoryWatcher.h>

class ezWindow;
class ezDirectoryWatcher;

/// Uses shader reloading mechanism of the ShaderExplorer sample for quick prototyping.
class ezComputeShaderHistogramApp : public ezGameApplication
{
public:
  typedef ezGameApplication SUPER;

  ezComputeShaderHistogramApp();
  ~ezComputeShaderHistogramApp();

  virtual ezApplication::ApplicationExecution Run() override;

  virtual void AfterCoreSystemsStartup() override;
  virtual void BeforeHighLevelSystemsShutdown() override;

private:

  void CreateHistogramQuad();
  void OnFileChanged(const char* filename, ezDirectoryWatcherAction action);

  ezGALTextureHandle m_hScreenTexture;
  ezGALRenderTargetViewHandle m_hScreenRTV;
  ezGALResourceViewHandle m_hScreenSRV;

  // Could use buffer, but access and organisation with texture is more straight forward.
  ezGALTextureHandle m_hHistogramTexture;
  ezGALUnorderedAccessViewHandle m_hHistogramUAV;
  ezGALResourceViewHandle m_hHistogramSRV;

  ezGALRenderTargetViewHandle m_hBackbufferRTV;

  ezShaderResourceHandle m_hScreenShader;
  ezShaderResourceHandle m_hHistogramDisplayShader;
  ezShaderResourceHandle m_hHistogramComputeShader;

  ezMeshBufferResourceHandle m_hHistogramQuadMeshBuffer;

  ezUniquePtr<ezDirectoryWatcher> m_directoryWatcher;

  bool m_stuffChanged;
};
