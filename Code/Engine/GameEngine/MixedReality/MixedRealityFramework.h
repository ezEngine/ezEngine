#pragma once

#include <GameEngine/Basics.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Types/UniquePtr.h>

#ifdef BUILDSYSTEM_ENABLE_MIXEDREALITY_SUPPORT

class ezCamera;
class ezWindowsHolographicSpace;
struct ezGameApplicationEvent;
class ezSurfaceReconstructionMeshManager;

class EZ_GAMEENGINE_DLL ezMixedRealityFramework
{
  EZ_DECLARE_SINGLETON(ezMixedRealityFramework);
public:

  ezMixedRealityFramework(ezCamera* pCameraForSynchronization /* = nullptr*/);
  ~ezMixedRealityFramework();

  /// \brief When an ezCamera is provided, the holographic space will automatically fill it with the
  /// latest prediction values at the right time every frame.
  void SetCameraForPredictionSynchronization(ezCamera* pCamera);

  ezSurfaceReconstructionMeshManager& GetSpatialMappingManager() const { return *m_pSpatialMappingManager; }

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