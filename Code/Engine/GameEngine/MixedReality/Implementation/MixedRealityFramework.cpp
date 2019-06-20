#include <GameEnginePCH.h>

#include <GameEngine/GameApplication/GameApplication.h>
#include <GameEngine/MixedReality/MixedRealityFramework.h>
#include <RendererCore/Pipeline/Declarations.h>

//////////////////////////////////////////////////////////////////////////

#ifdef BUILDSYSTEM_ENABLE_MIXEDREALITY_SUPPORT

#include <Core/World/World.h>
#include <Interfaces/SoundInterface.h>
#include <RendererCore/Components/CameraComponent.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererFoundation/Resources/RenderTargetSetup.h>
#include <WindowsMixedReality/Graphics/MixedRealityCamera.h>
#include <WindowsMixedReality/Graphics/MixedRealityDX11Device.h>
#include <WindowsMixedReality/HolographicSpace.h>
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
  ezGameApplication::SetOverrideDefaultDeviceCreator([this](const ezGALDeviceCreationDescription& desc) -> ezGALDevice* {
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
  ezGameApplicationBase::GetGameApplicationBaseInstance()->m_ExecutionEvents.RemoveEventHandler(
      ezMakeDelegate(&ezMixedRealityFramework::GameApplicationEventHandler, this));

  m_pSpatialMappingManager = nullptr;
  m_pHolospaceToDestroy = nullptr;
  m_pCameraToSynchronize = nullptr;
}

void ezMixedRealityFramework::GameApplicationEventHandler(const ezGameApplicationExecutionEvent& e)
{
  if (e.m_Type == ezGameApplicationExecutionEvent::Type::AfterWorldUpdates)
  {
    if (m_pCameraToSynchronize)
    {
      ezWindowsHolographicSpace::GetSingleton()->SynchronizeCameraPrediction(*m_pCameraToSynchronize);

      const ezMat4 mAdd = m_AdditionalCameraTransform.GetAsMat4();

      const ezMat4 viewL = m_pCameraToSynchronize->GetViewMatrix(ezCameraEye::Left) * mAdd;
      const ezMat4 viewR = m_pCameraToSynchronize->GetViewMatrix(ezCameraEye::Right) * mAdd;
      m_pCameraToSynchronize->SetViewMatrix(viewL, ezCameraEye::Left);
      m_pCameraToSynchronize->SetViewMatrix(viewR, ezCameraEye::Right);

      // put the camera orientation into the sound listener and enable the listener override mode
      if (ezSoundInterface* pSoundInterface = ezSingletonRegistry::GetSingletonInstance<ezSoundInterface>("ezSoundInterface"))
      {
        pSoundInterface->SetListener(-1, m_pCameraToSynchronize->GetCenterPosition(), m_pCameraToSynchronize->GetCenterDirForwards(),
                                     m_pCameraToSynchronize->GetCenterDirUp(), ezVec3::ZeroVector());
      }
    }
  }

  if (e.m_Type == ezGameApplicationExecutionEvent::Type::BeginAppTick)
  {
    ezWindowsHolographicSpace::GetSingleton()->ProcessAddedRemovedCameras();
  }
}

void ezMixedRealityFramework::OnDeviceCreated(bool bHolographicDevice)
{
  if (bHolographicDevice)
  {
    ezGameApplicationBase::GetGameApplicationBaseInstance()->m_ExecutionEvents.AddEventHandler(
        ezMakeDelegate(&ezMixedRealityFramework::GameApplicationEventHandler, this));

    m_pSpatialMappingManager = EZ_DEFAULT_NEW(ezSurfaceReconstructionMeshManager);
  }
}

void ezMixedRealityFramework::SetCameraForPredictionSynchronization(ezCamera* pCamera)
{
  if (m_pCameraToSynchronize == pCamera)
    return;

  EZ_ASSERT_DEV(pCamera == nullptr || pCamera->GetCameraMode() == ezCameraMode::Stereo, "Incorrect camera mode. Should be 'Stereo'.");

  m_pCameraToSynchronize = pCamera;
  m_AdditionalCameraTransform.SetIdentity();

  if (ezSoundInterface* pSoundInterface = ezSingletonRegistry::GetSingletonInstance<ezSoundInterface>("ezSoundInterface"))
  {
    if (pCamera != nullptr)
    {
      pSoundInterface->SetListenerOverrideMode(true);
    }
  }
}


void ezMixedRealityFramework::SetAdditionalCameraTransform(const ezTransform& transform)
{
  m_AdditionalCameraTransform = transform;
}

ezViewHandle ezMixedRealityFramework::CreateHolographicView(ezWindowBase* pWindow, const ezRenderPipelineResourceHandle& hRenderPipeline,
                                                            ezCamera* pCamera, ezWorld* pWorld /*= nullptr*/)
{
  auto pHoloSpace = ezWindowsHolographicSpace::GetSingleton();

  if (!pHoloSpace->IsAvailable())
  {
    ezLog::Error("ezMixedRealityFramework::CreateMainView: Holographic space is not available.");
    return ezViewHandle();
  }

  while (pHoloSpace->GetCameras().IsEmpty())
  {
    if (!ezGameApplicationBase::GetGameApplicationBaseInstance()->ProcessWindowMessages())
    {
      EZ_REPORT_FAILURE("No window has been added to ezGameApplication, thus no Holo CameraAdded events can arrive!");
      return ezViewHandle();
    }

    pHoloSpace->ProcessAddedRemovedCameras();
  }

  pCamera->SetCameraMode(ezCameraMode::Stereo, 90.0f, 0.1f, 100.0f);
  SetCameraForPredictionSynchronization(pCamera);

  auto hRemoteWindowSwapChain = ezGALDevice::GetDefaultDevice()->GetPrimarySwapChain();
  EZ_ASSERT_DEBUG(!hRemoteWindowSwapChain.IsInvalidated(),
                  "Primary swap chain is still invalid after a holographic camera has been added.");

  const ezGALSwapChain* pSwapChain = ezGALDevice::GetDefaultDevice()->GetSwapChain(hRemoteWindowSwapChain);
  auto hBackBufferRTV = ezGALDevice::GetDefaultDevice()->GetDefaultRenderTargetView(pSwapChain->GetBackBufferTexture());

  ezView* pMainView = nullptr;
  ezViewHandle hMainView = ezRenderWorld::CreateView("Holographic View", pMainView);
  pMainView->SetCameraUsageHint(ezCameraUsageHint::MainView);

  ezGALRenderTargetSetup renderTargetSetup;
  renderTargetSetup.SetRenderTarget(0, hBackBufferRTV);
  pMainView->SetRenderTargetSetup(renderTargetSetup);
  pMainView->SetRenderPipelineResource(hRenderPipeline);
  pMainView->SetCamera(pCamera);
  pMainView->SetViewport(pHoloSpace->GetCameras()[0]->GetViewport());
  pMainView->SetWorld(pWorld);

  ezRenderWorld::AddMainView(hMainView);

  ezGameApplicationBase::GetGameApplicationBaseInstance()->AddWindow(pWindow, hRemoteWindowSwapChain);

  return hMainView;
}


void ezMixedRealityFramework::SynchronizeCameraOrientationToCameraObjects(ezWorld* pWorld)
{
  ezCameraComponentManager* pCamMan = pWorld->GetComponentManager<ezCameraComponentManager>();
  if (pCamMan == nullptr)
    return;

  EZ_LOCK(pWorld->GetWriteMarker());

  for (auto it = pCamMan->GetComponents(); it.IsValid(); ++it)
  {
    if (it->IsActiveAndInitialized() && it->GetUsageHint() == ezCameraUsageHint::MainView)
    {
      ezGameObject* pOwner = it->GetOwner();

      SynchronizeCameraOrientationToGameObject(pOwner);
    }
  }
}

void ezMixedRealityFramework::SynchronizeCameraOrientationToGameObject(ezGameObject* pObject)
{
  auto pHoloSpace = ezWindowsHolographicSpace::GetSingleton();
  if (pHoloSpace->GetCameras().IsEmpty())
    return;

  if (m_pCameraToSynchronize == nullptr)
    return;

  ezTransform tGlobal;
  tGlobal.m_vPosition = m_pCameraToSynchronize->GetCenterPosition();
  tGlobal.m_vScale.Set(1.0f);

  ezMat3 mRot;
  mRot.SetColumn(0, m_pCameraToSynchronize->GetCenterDirForwards());
  mRot.SetColumn(1, m_pCameraToSynchronize->GetCenterDirRight());
  mRot.SetColumn(2, m_pCameraToSynchronize->GetCenterDirUp());

  tGlobal.m_qRotation.SetFromMat3(mRot);

  pObject->SetGlobalTransform(tGlobal);
}

#endif



EZ_STATICLINK_FILE(GameEngine, GameEngine_MixedReality_MixedRealityFramework);

