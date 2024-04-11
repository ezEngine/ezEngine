#pragma once

#include <Foundation/Application/Application.h>
#include <Foundation/IO/DirectoryWatcher.h>
#include <Foundation/Types/UniquePtr.h>
#include <RendererCore/Material/MaterialResource.h>
#include <RendererCore/Meshes/MeshResource.h>
#include <RendererFoundation/RendererFoundationDLL.h>

// define this to force usage of fileserve functionality
// #define USE_FILESERVE EZ_ON

// to use fileserve, run the ezFileServe application with a command line that tells it where the ":project"
// data directory is located on the PC, for example:
//
// ezFileServe.exe -fs_start -specialdirs project "C:\ez\Data\Samples\ShaderExplorer"

#if !defined(USE_FILESERVE)

#  if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT) && EZ_DISABLED(EZ_SUPPORTS_UNRESTRICTED_FILE_ACCESS)
// on sandboxed platforms, we can only load data through fileserve, so enforce use of this plugin
#    define USE_FILESERVE EZ_ON
#  else
#    define USE_FILESERVE EZ_OFF
#  endif

#endif


#if EZ_DISABLED(USE_FILESERVE) && EZ_ENABLED(EZ_SUPPORTS_DIRECTORY_WATCHER)
#  define USE_DIRECTORY_WATCHER EZ_ON
#else
#  define USE_DIRECTORY_WATCHER EZ_OFF
#endif

class ezShaderExplorerWindow;
class ezCamera;
class ezGALDevice;
class ezDirectoryWatcher;
class ezVirtualThumbStick;

// A simple application that renders a full screen quad with a single shader and does live reloading of that shader
// Can be used for singed distance rendering experiments or other single shader experiments.
class ezShaderExplorerApp : public ezApplication
{
public:
  using SUPER = ezApplication;

  ezShaderExplorerApp();

  virtual Execution Run() override;

  virtual void AfterCoreSystemsStartup() override;

  virtual void BeforeHighLevelSystemsShutdown() override;

private:
  void UpdateSwapChain();
  void CreateScreenQuad();

#if EZ_ENABLED(USE_DIRECTORY_WATCHER)
  ezUniquePtr<ezDirectoryWatcher> m_pDirectoryWatcher;
  void OnFileChanged(ezStringView sFilename, ezDirectoryWatcherAction action, ezDirectoryWatcherType type);
#endif

  ezShaderExplorerWindow* m_pWindow = nullptr;
  ezGALDevice* m_pDevice = nullptr;

  ezGALSwapChainHandle m_hSwapChain;
  ezGALTextureHandle m_hDepthStencilTexture;

  ezMaterialResourceHandle m_hMaterial;
  ezMeshBufferResourceHandle m_hQuadMeshBuffer;

  ezUniquePtr<ezCamera> m_pCamera;
  ezUniquePtr<ezVirtualThumbStick> m_pLeftStick;
  ezUniquePtr<ezVirtualThumbStick> m_pRightStick;

  bool m_bStuffChanged;
};
