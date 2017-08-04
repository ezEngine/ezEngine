#include <PCH.h>
#include <GameEngine/MixedReality/MixedRealityFramework.h>
#include <GameEngine/GameApplication/GameApplication.h>

//////////////////////////////////////////////////////////////////////////

#ifdef BUILDSYSTEM_ENABLE_MIXEDREALITY_SUPPORT

#include <WindowsMixedReality/HolographicSpace.h>
#include <WindowsMixedReality/Graphics/MixedRealityDX11Device.h>
#include <WindowsMixedReality/SpatialMapping/SurfaceReconstructionMeshManager.h>

EZ_IMPLEMENT_SINGLETON(ezMixedRealityFramework);

ezMixedRealityFramework::ezMixedRealityFramework(ezCamera* pCameraForSynchronization)
  : m_SingletonRegistrar(this)
{
  Startup(pCameraForSynchronization);
}

ezMixedRealityFramework::~ezMixedRealityFramework()
{
  Shutdown();
}

void ezMixedRealityFramework::Startup(ezCamera* pCameraForSynchronization)
{
  ezGameApplication::SetOverrideDefaultDeviceCreator([this](const ezGALDeviceCreationDescription& desc) -> ezGALDevice*
  {
    auto pHoloSpace = ezWindowsHolographicSpace::GetSingleton();
    if (pHoloSpace->IsAvailable())
    {
      ezGALDevice* pDevice = EZ_DEFAULT_NEW(ezGALMixedRealityDeviceDX11, desc);
      OnDeviceCreated(true);
      return pDevice;
    }
    else
    {
      ezGALDevice* pDevice = EZ_DEFAULT_NEW(ezGALDeviceDX11, desc);
      OnDeviceCreated(false);
      return pDevice;
    }
  });

  auto pHolospace = ezWindowsHolographicSpace::GetSingleton();

  if (pHolospace == nullptr)
  {
    m_pHolospaceToDestroy = EZ_DEFAULT_NEW(ezWindowsHolographicSpace);
    pHolospace = m_pHolospaceToDestroy.Borrow();
    pHolospace->InitForMainCoreWindow();
  }

  if (pHolospace->GetDefaultReferenceFrame() == nullptr)
  {
    pHolospace->CreateDefaultReferenceFrame();
  }

  SetCameraForPredictionSynchronization(pCameraForSynchronization);
}

void ezMixedRealityFramework::Shutdown()
{
  ezGameApplication::GetGameApplicationInstance()->m_Events.RemoveEventHandler(ezMakeDelegate(&ezMixedRealityFramework::GameApplicationEventHandler, this));

  m_pSpatialMappingManager = nullptr;
  m_pHolospaceToDestroy = nullptr;
  m_pCameraToSynchronize = nullptr;
}

void ezMixedRealityFramework::GameApplicationEventHandler(const ezGameApplicationEvent& e)
{
  if (e.m_Type == ezGameApplicationEvent::Type::AfterWorldUpdates)
  {
    if (m_pCameraToSynchronize)
    {
      ezWindowsHolographicSpace::GetSingleton()->SynchronizeCameraPrediction(*m_pCameraToSynchronize);
    }
  }

  if (e.m_Type == ezGameApplicationEvent::Type::BeginAppTick)
  {
    ezWindowsHolographicSpace::GetSingleton()->ProcessAddedRemovedCameras();
  }
}


void ezMixedRealityFramework::OnDeviceCreated(bool bHolographicDevice)
{
  if (bHolographicDevice)
  {
    ezGameApplication::GetGameApplicationInstance()->m_Events.AddEventHandler(ezMakeDelegate(&ezMixedRealityFramework::GameApplicationEventHandler, this));

    m_pSpatialMappingManager = EZ_DEFAULT_NEW(ezSurfaceReconstructionMeshManager);
  }
}

void ezMixedRealityFramework::SetCameraForPredictionSynchronization(ezCamera* pCamera)
{
  if (m_pCameraToSynchronize == pCamera)
    return;

  EZ_ASSERT_DEV(pCamera == nullptr || pCamera->GetCameraMode() == ezCameraMode::Stereo, "Incorrect camera mode. Should be 'Stereo'.");
  m_pCameraToSynchronize = pCamera;
}

#endif