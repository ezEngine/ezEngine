#include <PCH.h>
#include <WindowsMixedReality/MixedRealityGameState.h>
#include <WindowsMixedReality/HolographicSpace.h>
#include <WindowsMixedReality/Graphics/MixedRealityDX11Device.h>
#include <WindowsMixedReality/Graphics/MixedRealitySwapChainDX11.h>

#include <GameEngine/GameApplication/GameApplication.h>
#include <GameEngine/GameState/GameStateWindow.h>

#include <Foundation/Threading/Lock.h>
#include <Core/World/World.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMixedRealityGameState, 1, ezRTTIDefaultAllocator<ezMixedRealityGameState>);
EZ_END_DYNAMIC_REFLECTED_TYPE

void ezMixedRealityGameState::OnActivation(ezWorld* pWorld)
{
  m_bStateWantsToQuit = false;
  m_pMainWorld = pWorld;

  m_pMainWindow = EZ_DEFAULT_NEW(ezGameStateWindow, ezWindowCreationDesc());
  GetApplication()->AddWindow(m_pMainWindow, ezGALSwapChainHandle());

  {
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

void ezMixedRealityGameState::ProcessInput()
{
}

void ezMixedRealityGameState::BeforeWorldUpdate()
{
  EZ_LOCK(m_pMainWorld->GetReadMarker());
}

void ezMixedRealityGameState::AfterWorldUpdate()
{
  EZ_LOCK(m_pMainWorld->GetReadMarker());
}

void ezMixedRealityGameState::ConfigureInputActions()
{
}

void ezMixedRealityGameState::OnHolographicCameraAdded(const ezWindowsMixedRealityCamera& camera)
{
  if (m_hMainSwapChain.IsInvalidated())
  {
    m_hMainSwapChain = ezGALDevice::GetDefaultDevice()->GetPrimarySwapChain();
    EZ_ASSERT_DEBUG(!m_hMainSwapChain.IsInvalidated(), "Primary swap chain is still invalid after a holographic camera has been added.");

    const ezGALSwapChain* pSwapChain = ezGALDevice::GetDefaultDevice()->GetSwapChain(m_hMainSwapChain);
    SetupMainView(ezGALDevice::GetDefaultDevice()->GetDefaultRenderTargetView(pSwapChain->GetBackBufferTexture()));

    GetApplication()->SetSwapChain(m_pMainWindow, m_hMainSwapChain);
  }
  else
  {
    ezLog::Warning("New holographic camera was added but ezMixedRealityGameState supports currently only a single holographic camera!");
  }
}
