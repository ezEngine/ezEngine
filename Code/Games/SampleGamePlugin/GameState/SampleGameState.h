#pragma once

#include <SampleGamePlugin/SampleGamePluginDLL.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <GameEngine/GameState/GameState.h>
#include <GameEngine/GameState/FallbackGameState.h>
#include <Core/World/Declarations.h>
#include <Core/Input/Declarations.h>

class EZ_SAMPLEGAMEPLUGIN_DLL SampleGameState : public ezFallbackGameState
{
  EZ_ADD_DYNAMIC_REFLECTION(SampleGameState, ezFallbackGameState);

public:
  SampleGameState();

  virtual 

ezGameStatePriority DeterminePriority(ezWorld* pWorld) const override;

  virtual void ProcessInput() override;

protected:
  virtual void ConfigureInputDevices() override;
  virtual void ConfigureInputActions() override;
  virtual void ConfigureMainCamera() override;

private:
  virtual void OnActivation(ezWorld* pWorld, const ezTransform* pStartPosition) override;
  virtual void OnDeactivation() override;
  virtual void BeforeWorldUpdate() override;




};
