#pragma once

#include <Core/Basics.h>

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

  ezWorld* GetWorld() const { return m_pWorld; }

protected:
  void CreateSceneModules();
  void DestroySceneModules();


  ezHybridArray<ezSceneModule*, 8> m_SceneModules;
  ezWorld* m_pWorld;
};

