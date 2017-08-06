#pragma once

#include <GameEngine/Basics.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Types/UniquePtr.h>
#include <Core/ResourceManager/ResourceHandle.h>
#include <RendererCore/Pipeline/Declarations.h>

#ifdef BUILDSYSTEM_ENABLE_MIXEDREALITY_SUPPORT

class ezCamera;
class ezWindowsHolographicSpace;
struct ezGameApplicationEvent;
class ezSurfaceReconstructionMeshManager;
class ezViewHandle;
class ezWindowBase;
typedef ezTypedResourceHandle<class ezRenderPipelineResource> ezRenderPipelineResourceHandle;

class EZ_GAMEENGINE_DLL ezMixedRealityFramework
{
  EZ_DECLARE_SINGLETON(ezMixedRealityFramework);
public:

  ezMixedRealityFramework(ezCamera* pCameraForSynchronization /* = nullptr*/);
  ~ezMixedRealityFramework();

  /// \brief When an ezCamera is provided, the holographic space will automatically fill it with the
  /// latest prediction values at the right time every frame.
  void SetCameraForPredictionSynchronization(ezCamera* pCamera);

  ezCamera* GetCameraForPredictionSynchronization() { return m_pCameraToSynchronize; }

  ezSurfaceReconstructionMeshManager& GetSpatialMappingManager() const { return *m_pSpatialMappingManager; }

  /// \brief Attempts to create the main render view for a mixed reality application.
  ///
  /// The view is configured to use the given render pipeline and have the MR camera viewport.
  /// It is added to ezRenderWorld as a main view.
  /// The given window is added to ezGameApplication, using AddWindow, together with the main view's swap chain.
  /// 
  /// The function also sets the camera as the camera that gets automatic prediction synchronization,
  /// and it makes sure the camera is set to be in 'Stereo' mode.
  /// 
  /// If the function fails, an invalid handle is returned.
  /// This happens either when the holographic space is not available or no MR camera was given by the OS.
  ezViewHandle CreateHolographicView(ezWindowBase* pWindow, const ezRenderPipelineResourceHandle& hRenderPipeline, ezCamera* pCamera, ezWorld* pWorld = nullptr);

  void SynchronizeCameraOrientationToCameraObjects(ezWorld* pWorld);
  void SynchronizeCameraOrientationToGameObject(ezGameObject* pObject);

private:
  void Startup(ezCamera* pCameraForSynchronization);
  void Shutdown();
  void GameApplicationEventHandler(const ezGameApplicationEvent& e);
  void OnDeviceCreated(bool bHolographicDevice);

  // the camera object that will get the head tracking data applied every frame
  ezCamera* m_pCameraToSynchronize = nullptr;

  // only set when the MR framework created it itself
  ezUniquePtr<ezWindowsHolographicSpace> m_pHolospaceToDestroy;

  ezUniquePtr<ezSurfaceReconstructionMeshManager> m_pSpatialMappingManager;
};

#endif