#include <PCH.h>
#include <GameEngine/MixedReality/MixedRealityGameState.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <GameEngine/GameState/GameStateWindow.h>

#include <RendererCore/Components/CameraComponent.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererCore/Pipeline/View.h>

#include <Foundation/Threading/Lock.h>
#include <Core/World/World.h>

#include <windows.perception.spatial.h>
#include <Core/Input/InputManager.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMixedRealityGameState, 1, ezRTTIDefaultAllocator<ezMixedRealityGameState>);
EZ_END_DYNAMIC_REFLECTED_TYPE

ezMixedRealityGameState::ezMixedRealityGameState()
{}

ezMixedRealityGameState::~ezMixedRealityGameState()
{}



#ifdef BUILDSYSTEM_ENABLE_MIXEDREALITY_SUPPORT

#include <WindowsMixedReality/HolographicSpace.h>
#include <WindowsMixedReality/SpatialLocationService.h>
#include <WindowsMixedReality/SpatialReferenceFrame.h>
#include <WindowsMixedReality/Graphics/MixedRealityCamera.h>
#include <WindowsMixedReality/Graphics/MixedRealityDX11Device.h>
#include <WindowsMixedReality/Graphics/MixedRealitySwapChainDX11.h>
#include <WindowsMixedReality/SpatialMapping/SurfaceReconstructionMeshManager.h>
#include <GameEngine/MixedReality/MixedRealityFramework.h>


void ezMixedRealityGameState::OnActivation(ezWorld* pWorld)
{
  m_bStateWantsToQuit = false;
  m_pMainWorld = pWorld;

  CreateMainWindow();

  ezMixedRealityFramework::GetSingleton()->SetCameraForPredictionSynchronization(&m_MainCamera);
  auto pHoloSpace = ezWindowsHolographicSpace::GetSingleton();

  // Holographic/Stereo!
  {
    pHoloSpace->m_cameraAddedEvent.AddEventHandler(ezMakeDelegate(&ezMixedRealityGameState::OnHolographicCameraAdded, this));
  }

  ConfigureMainCamera();

  ConfigureInputDevices();

  ConfigureInputActions();
}

void ezMixedRealityGameState::OnDeactivation()
{
  auto pHoloSpace = ezWindowsHolographicSpace::GetSingleton();

  pHoloSpace->m_cameraAddedEvent.RemoveEventHandler(ezMakeDelegate(&ezMixedRealityGameState::OnHolographicCameraAdded, this));
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
  ezFallbackGameState::SetupMainView(hBackBuffer, ezResourceManager::LoadResource<ezRenderPipelineResource>("{ 2fe25ded-776c-7f9e-354f-e4c52a33d125 }"));
}

void ezMixedRealityGameState::ProcessInput()
{
  const char* szPressed = ezInputManager::GetPressedInputSlot(ezInputSlotFlags::None, ezInputSlotFlags::None);

  if (!ezStringUtils::IsNullOrEmpty(szPressed))
  {
    ezLog::Info("Pressed: {0}", szPressed);

    ezMixedRealityFramework::GetSingleton()->GetSpatialMappingManager().PullCurrentSurfaces();
  }
}

void ezMixedRealityGameState::ConfigureInputActions()
{
  // skip the base class input action configuration
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

#endif

