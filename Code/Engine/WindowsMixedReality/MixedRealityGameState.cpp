#include <PCH.h>
#include <WindowsMixedReality/MixedRealityGameState.h>
#include <WindowsMixedReality/HolographicSpace.h>
#include <WindowsMixedReality/SpatialLocationService.h>
#include <WindowsMixedReality/SpatialReferenceFrame.h>
#include <WindowsMixedReality/Graphics/MixedRealityCamera.h>
#include <WindowsMixedReality/Graphics/MixedRealityDX11Device.h>
#include <WindowsMixedReality/Graphics/MixedRealitySwapChainDX11.h>

#include <GameEngine/GameApplication/GameApplication.h>
#include <GameEngine/GameState/GameStateWindow.h>

#include <RendererCore/Components/CameraComponent.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererCore/Pipeline/View.h>

#include <Foundation/Threading/Lock.h>
#include <Core/World/World.h>

#include <windows.perception.spatial.h>


EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMixedRealityGameState, 1, ezRTTIDefaultAllocator<ezMixedRealityGameState>);
EZ_END_DYNAMIC_REFLECTED_TYPE

ezMixedRealityGameState::ezMixedRealityGameState()
{}

ezMixedRealityGameState::~ezMixedRealityGameState()
{}

void ezMixedRealityGameState::OnActivation(ezWorld* pWorld)
{
  m_bStateWantsToQuit = false;
  m_pMainWorld = pWorld;

  m_pMainWindow = EZ_DEFAULT_NEW(ezGameStateWindow, ezWindowCreationDesc());
  GetApplication()->AddWindow(m_pMainWindow, ezGALSwapChainHandle());

  // Holographic/Stereo!
  {
    m_pDefaultReferenceFrame = ezWindowsHolographicSpace::GetSingleton()->GetSpatialLocationService().CreateStationaryReferenceFrame_CurrentLocation();

    ezWindowsHolographicSpace::GetSingleton()->m_cameraAddedEvent.AddEventHandler(ezMakeDelegate(&ezMixedRealityGameState::OnHolographicCameraAdded, this));

    // Need to handle add/remove cameras before anything else - world update won't happen without a view wich may be created/destroyed by this.
    GetApplication()->m_Events.AddEventHandler([this](const ezGameApplicationEvent& evt)
    {
      if (evt.m_Type == ezGameApplicationEvent::Type::BeginFrame)
        ezWindowsHolographicSpace::GetSingleton()->ProcessAddedRemovedCameras();
    });
  }

  ConfigureMainCamera();

  ConfigureInputDevices();

  ConfigureInputActions();
}

void ezMixedRealityGameState::OnDeactivation()
{
  ezWindowsHolographicSpace::GetSingleton()->m_cameraAddedEvent.RemoveEventHandler(ezMakeDelegate(&ezMixedRealityGameState::OnHolographicCameraAdded, this));
  m_pDefaultReferenceFrame.Reset();
}

ezGALDevice* ezMixedRealityGameState::CreateGraphicsDevice(const ezGALDeviceCreationDescription& description)
{
  return EZ_DEFAULT_NEW(ezGALMixedRealityDeviceDX11, description);
}

float ezMixedRealityGameState::CanHandleThis(ezGameApplicationType AppType, ezWorld* pWorld) const
{
  if (pWorld == nullptr)
    return -1.0f;

  if (ezWindowsHolographicSpace::GetSingleton()->IsAvailable())
    return -1.0f;

  return 1.0f;
}

void ezMixedRealityGameState::SetupMainView(ezGALRenderTargetViewHandle hBackBuffer)
{
  // HololensRenderPipeline.ezRendePipelineAsset
  ezFallbackGameState::SetupMainView(hBackBuffer, ezResourceManager::LoadResource<ezRenderPipelineResource>("{ f94d0bf2-e90a-49da-a356-b21abd545997 }"));
}

void ezMixedRealityGameState::ProcessInput()
{
}

void ezMixedRealityGameState::BeforeWorldUpdate()
{
  ezFallbackGameState::BeforeWorldUpdate();
}

void ezMixedRealityGameState::AfterWorldUpdate()
{
  EZ_LOCK(m_pMainWorld->GetReadMarker());

  // Update the camera transform after world update so the owner node has its final position for this frame.
  // Setting the camera transform in ProcessInput introduces one frame delay.
  auto holoCameras = ezWindowsHolographicSpace::GetSingleton()->GetCameras();
  if (!holoCameras.IsEmpty())
  {
    ezWindowsMixedRealityCamera* pHoloCamera = holoCameras[0];
    
    auto viewport = pHoloCamera->GetViewport();
    m_MainCamera.SetStereoProjection(pHoloCamera->GetProjectionLeft(), pHoloCamera->GetProjectionRight(), viewport.width / viewport.height);
    
    ezMat4 mViewTransformLeft, mViewTransformRight;
    pHoloCamera->GetViewTransforms(*m_pDefaultReferenceFrame, mViewTransformLeft, mViewTransformRight);
    m_MainCamera.SetViewMatrix(mViewTransformLeft, ezCameraEye::Left);
    m_MainCamera.SetViewMatrix(mViewTransformRight, ezCameraEye::Right);

    // If there is an active camera component we update its position, but technically we don't need one!
    /*if (ezCameraComponent* pCamComp = FindActiveCameraComponent())
    {
      ezGameObject* pOwner = pCamComp->GetOwner();
      pOwner->SetGlobalPosition(m_MainCamera.GetCenterPosition());

      ezMat3 mRotation;
      mRotation.SetLookInDirectionMatrix(m_MainCamera.GetCenterDirForwards(), m_MainCamera.GetCenterDirUp());
      ezQuat rotationQuat;
      rotationQuat.SetFromMat3(mRotation);
      pOwner->SetGlobalRotation(rotationQuat);
    }*/
  }
}

void ezMixedRealityGameState::ConfigureInputActions()
{
}

void ezMixedRealityGameState::OnHolographicCameraAdded(const ezWindowsMixedRealityCamera& camera)
{
  if (m_hMainSwapChain.IsInvalidated())
  {
    // Set camera to stereo immediately so, the view is setup correctly.
    m_MainCamera.SetCameraMode(ezCameraMode::Stereo, 1.0f, 1.0f, 1000.0f);

    m_hMainSwapChain = ezGALDevice::GetDefaultDevice()->GetPrimarySwapChain();
    EZ_ASSERT_DEBUG(!m_hMainSwapChain.IsInvalidated(), "Primary swap chain is still invalid after a holographic camera has been added.");

    const ezGALSwapChain* pSwapChain = ezGALDevice::GetDefaultDevice()->GetSwapChain(m_hMainSwapChain);
    SetupMainView(ezGALDevice::GetDefaultDevice()->GetDefaultRenderTargetView(pSwapChain->GetBackBufferTexture()));

    // Viewport is different from window size (which is what SetupMainView will use), need to set manually!
    ezView* pView = nullptr;
    ezRenderWorld::TryGetView(m_hMainView, pView);
    pView->SetViewport(camera.GetViewport());

    GetApplication()->SetSwapChain(m_pMainWindow, m_hMainSwapChain);
  }
  else
  {
    ezLog::Warning("New holographic camera was added but ezMixedRealityGameState supports currently only a single holographic camera!");
  }
}
