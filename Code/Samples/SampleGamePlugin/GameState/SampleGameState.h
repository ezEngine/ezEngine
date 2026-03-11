#pragma once

#include <Core/Input/Declarations.h>
#include <Core/World/Declarations.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <GameEngine/GameState/FallbackGameState.h>
#include <GameEngine/GameState/GameState.h>
#include <SampleGamePlugin/SampleGamePluginDLL.h>

class EZ_SAMPLEGAMEPLUGIN_DLL SampleGameState : public ezFallbackGameState
{
  EZ_ADD_DYNAMIC_REFLECTION(SampleGameState, ezFallbackGameState);

public:
  SampleGameState();

  virtual void ProcessInput() override;

protected:
  virtual void ConfigureMainWindowInputDevices(ezWindow* pWindow) override;
  virtual void ConfigureInputActions() override;
  virtual void ConfigureMainCamera() override;

private:
  virtual void OnActivation(ezWorld* pWorld, ezStringView sStartPosition, const ezTransform& startPositionOffset) override;
  virtual void OnDeactivation() override;
  virtual void BeforeWorldUpdate() override;
  virtual void AfterWorldUpdate() override;

  // BEGIN-DOCS-CODE-SNIPPET: confunc-decl
  void ConFunc_Print(ezString sText);
  ezConsoleFunction<void(ezString)> m_ConFunc_Print;
  // END-DOCS-CODE-SNIPPET

  ezDeque<ezGameObjectHandle> m_SpawnedObjects;
};
