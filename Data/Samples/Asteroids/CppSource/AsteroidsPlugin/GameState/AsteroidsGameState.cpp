#include <AsteroidsPlugin/AsteroidsPluginPCH.h>

#include <AsteroidsPlugin/GameState/AsteroidsGameState.h>
#include <Foundation/Utilities/CommandLineUtils.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(AsteroidsGameState, 1, ezRTTIDefaultAllocator<AsteroidsGameState>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

AsteroidsGameState::AsteroidsGameState() = default;
AsteroidsGameState::~AsteroidsGameState() = default;

void AsteroidsGameState::GetStartupOptions(ezString& out_sScene, ezString& out_sPreloadCollection)
{
  // if we have a "-scene" command line argument, it was launched from the editor and we should load that
  // otherwise, we use the hardcoded 'Main.ezScene'
  if (ezCommandLineUtils::GetGlobalInstance()->HasOption("-scene"))
  {
    out_sScene = ezCommandLineUtils::GetGlobalInstance()->GetStringOption("-scene");
  }
  else
  {
    out_sScene = "AssetCache/Common/Scenes/Main.ezBinScene";
  }
}
