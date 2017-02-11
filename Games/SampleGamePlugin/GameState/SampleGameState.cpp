#include <PCH.h>
#include <SampleGamePlugin/GameState/SampleGameState.h>
#include <Foundation/Logging/Log.h>
#include <Core/World/World.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(SampleGameState, 1, ezRTTIDefaultAllocator<SampleGameState>)
EZ_END_DYNAMIC_REFLECTED_TYPE

SampleGameState::SampleGameState()
{
}

void SampleGameState::OnActivation(ezWorld* pWorld)
{
  EZ_LOG_BLOCK("GameState::Activate");

  SUPER::OnActivation(pWorld);


}

void SampleGameState::OnDeactivation()
{
  EZ_LOG_BLOCK("GameState::Deactivate");

  SUPER::OnDeactivation();
}

void SampleGameState::BeforeWorldUpdate()
{
  EZ_LOCK(m_pMainWorld->GetWriteMarker());

}

float SampleGameState::CanHandleThis(ezGameApplicationType AppType, ezWorld* pWorld) const
{
  return 2.0f;
}

void SampleGameState::ConfigureInputDevices()
{
  // ezFallbackGameState
  SUPER::ConfigureInputDevices();

  // setup devices here
}

void SampleGameState::ConfigureInputActions()
{
  // ezFallbackGameState
  SUPER::ConfigureInputActions();

  // setup custom input actions here
}

void SampleGameState::ProcessInput()
{
  // ezFallbackGameState
  SUPER::ProcessInput();

  // do game input processing here
}

void SampleGameState::ConfigureMainCamera()
{
  // ezFallbackGameState
  SUPER::ConfigureMainCamera();

  // do custom camera setup here
}
