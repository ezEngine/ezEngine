#pragma once

#include <Core/Basics.h>

class ezWorld;
class ezSceneModule;

class EZ_CORE_DLL ezScene
{
public:
  ezScene();
  virtual ~ezScene();

  void Initialize();
  void Deinitialize();
  void Update();


protected:
  void CreateSceneModules();
  void DestroySceneModules();


  ezHybridArray<ezSceneModule*, 8> m_SceneModules;
  //ezWorld* m_pWorld;
};

