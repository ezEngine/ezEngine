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

// A simple application that renders a full screen quad with a single shader and does live reloading of that shader
// Can be used for singed distance rendering experiments or other single shader experiments.
class ezShaderExplorerApp : public ezApplication
{
public:
  ezShaderExplorerApp();
  ~ezShaderExplorerApp();

  virtual ApplicationExecution Run() override;

  virtual void AfterCoreStartup() override;

  virtual void BeforeCoreShutdown() override;

private:

  void CreateScreenQuad();
  void OnFileChanged(const char* filename, ezDirectoryWatcherAction action);

  ezShaderExplorerWindow* m_pWindow;
  ezGALDevice* m_pDevice;

  ezGALRenderTargetViewHandle m_hBBRTV;
  ezGALRenderTargetViewHandle m_hBBDSV;
  ezGALTextureHandle m_hDepthStencilTexture;

  ezMaterialResourceHandle m_hMaterial;
  ezMeshBufferResourceHandle m_hQuadMeshBuffer;

  ezUniquePtr<ezCamera> m_camera;
  ezUniquePtr<ezDirectoryWatcher> m_directoryWatcher;

  bool m_stuffChanged;
};
