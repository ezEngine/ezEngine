#pragma once

#include <Core/Input/Declarations.h>
#include <Core/World/Declarations.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <GameEngine/GameState/FallbackGameState.h>
#include <GameEngine/GameState/GameState.h>
#include <!CppProject!Plugin/!CppProject!PluginDLL.h>

class !CppProject!GameState : public ezFallbackGameState
{
  EZ_ADD_DYNAMIC_REFLECTION(!CppProject!GameState, ezFallbackGameState);

public:
  !CppProject!GameState();
  ~!CppProject!GameState();

  virtual ezGameStatePriority DeterminePriority(ezWorld* pWorld) const override;

  virtual void ProcessInput() override;

protected:
  virtual void ConfigureMainWindowInputDevices(ezWindow* pWindow) override;
  virtual void ConfigureInputActions() override;
  virtual void ConfigureMainCamera() override;

private:
  virtual void OnActivation(ezWorld* pWorld, const ezTransform* pStartPosition) override;
  virtual void OnDeactivation() override;
  virtual void BeforeWorldUpdate() override;
  virtual void AfterWorldUpdate() override;

  ezDeque<ezGameObjectHandle> m_SpawnedObjects;
};
