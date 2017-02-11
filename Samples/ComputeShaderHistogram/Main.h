#pragma once

#include <GameEngine/GameApplication/GameApplication.h>
#include <Foundation/Types/UniquePtr.h>
#include <RendererCore/Meshes/MeshResource.h>
#include <RendererCore/Material/MaterialResource.h>
#include <Foundation/IO/DirectoryWatcher.h>

class ezWindow;
class ezDirectoryWatcher;
class ezGPUStopwatch;

/// Uses shader reloading mechanism of the ShaderExplorer sample for quick prototyping.
class ezComputeShaderHistogramApp : public ezGameApplication
{
public:
  ezComputeShaderHistogramApp();
  ~ezComputeShaderHistogramApp();

  ezApplication::ApplicationExecution Run();

  virtual void AfterCoreStartup() override;
  virtual void BeforeCoreShutdown() override;

private:

  void CreateHistogramQuad();
  void OnFileChanged(const char* filename, ezDirectoryWatcherAction action);

  ezUniquePtr<ezWindow> m_pWindow;

  ezUniquePtr<ezGPUStopwatch> m_pHistogramGPUStopwatch;

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
