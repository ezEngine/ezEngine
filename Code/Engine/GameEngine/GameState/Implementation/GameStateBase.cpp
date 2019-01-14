#include <PCH.h>

#include <GameEngine/GameState/GameStateBase.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezGameStateBase, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezGameStateBase::ezGameStateBase() = default;
ezGameStateBase::~ezGameStateBase() = default;

void ezGameStateBase::ProcessInput() {}

void ezGameStateBase::BeforeWorldUpdate() {}

void ezGameStateBase::AfterWorldUpdate() {}

void ezGameStateBase::RequestQuit()
{
  m_bStateWantsToQuit = true;
}

bool ezGameStateBase::WasQuitRequested() const
{
  return m_bStateWantsToQuit;
}
