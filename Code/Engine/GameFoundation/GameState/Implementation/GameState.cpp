
#include <GameFoundation/PCH.h>
#include <GameFoundation/GameState/GameState.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezGameState, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();



EZ_STATICLINK_FILE(GameFoundation, GameFoundation_GameState);


ezGameState::ezGameState() 
  : m_pApplication(nullptr)
{

}

ezGameState::~ezGameState()
{

}

