#pragma once

#include <Core/Application/Application.h>
#include <Foundation/Types/UniquePtr.h>
#include <RendererFoundation/Basics.h>
#include <RendererCore/Meshes/MeshResource.h>
#include <RendererCore/Material/MaterialResource.h>
#include <Foundation/IO/DirectoryWatcher.h>

class ezShaderExplorerWindow;
class ezCamera;
class ezGALDevice;
class ezDirectoryWatcher;

/// Uses shader reloading mechanism of the ShaderExplorer sample for quick prototyping.
class ezComputeShaderHistogramApp : public ezApplication
{
public:
  ezComputeShaderHistogramApp();
  ~ezComputeShaderHistogramApp();

  virtual ApplicationExecution Run() override;

  virtual void AfterCoreStartup() override;
  virtual void BeforeCoreShutdown() override;

private:

  void CreateHistogramQuad();
  void OnFileChanged(const char* filename, ezDirectoryWatcherAction action);

  ezShaderExplorerWindow* m_pWindow;
  ezGALDevice* m_pDevice;

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
