#pragma once

#include <Core/Basics.h>
#include <Foundation/Time/DefaultTimeStepSmoothing.h>

class ezWorld;
class ezSceneModule;

class EZ_CORE_DLL ezScene
{
public:
  ezScene();
  virtual ~ezScene();

  void Initialize(const char* szWorldName);
  void Deinitialize();
  void Update();
  void ReinitSceneModules();

  const ezWorld* GetWorld() const { return m_pWorld; }
  ezWorld* GetWorld() { return m_pWorld; }

protected:
  void CreateSceneModules();
  void DestroySceneModules();

  ezDefaultTimeStepSmoothing m_TimeStepSmoothing;
  ezHybridArray<ezSceneModule*, 8> m_SceneModules;
  ezWorld* m_pWorld;
};

