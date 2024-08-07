#pragma once

#include <Core/Input/Declarations.h>
#include <Core/World/Declarations.h>
#include <CppProjectPlugin/CppProjectPluginDLL.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <GameEngine/GameState/FallbackGameState.h>
#include <GameEngine/GameState/GameState.h>

// the ezFallbackGameState adds a free flying camera and a scene switching menu, so can be useful in the very beginning
// but generally it's better to use ezGameState instead
// using CppProjectGameStateBase = ezFallbackGameState;
using CppProjectGameStateBase = ezGameState;

class CppProjectGameState : public CppProjectGameStateBase
{
  EZ_ADD_DYNAMIC_REFLECTION(CppProjectGameState, CppProjectGameStateBase);

public:
  CppProjectGameState();
  ~CppProjectGameState();

  virtual void ProcessInput() override;

protected:
  virtual void ConfigureInputActions() override;
  virtual void ConfigureMainCamera() override;
  virtual ezResult SpawnPlayer(ezStringView sStartPosition, const ezTransform* pStartPosition) override;
  virtual void OnChangedMainWorld(ezWorld* pPrevWorld, ezWorld* pNewWorld, ezStringView sStartPosition, const ezTransform* pStartPosition) override;
  virtual ezString GetStartupSceneFile() override;

private:
  virtual void OnActivation(ezWorld* pWorld, ezStringView sStartPosition, const ezTransform* pStartPosition) override;
  virtual void BeforeWorldUpdate() override;
  virtual void AfterWorldUpdate() override;

  ezDeque<ezGameObjectHandle> m_SpawnedObjects;
};
