#pragma once

#include <GameUtils/Basics.h>
#include <Foundation/Containers/Deque.h>
#include <Core/World/GameObject.h>
#include <Core/World/World.h>

class EZ_GAMEUTILS_DLL ezObjectSelection
{
public:
  ezObjectSelection();

  void SetWorld(ezWorld* pWorld);

  const ezWorld* GetWorld() const { return m_pWorld; }

  void Clear() { m_Objects.Clear(); }

  void RemoveDeadObjects();

  void AddObject(ezGameObjectHandle hObject);

  void RemoveObject(ezGameObjectHandle hObject);

  void ToggleSelection(ezGameObjectHandle hObject);

  ezUInt32 GetCount() const { return m_Objects.GetCount(); }

  ezGameObjectHandle GetObject(ezUInt32 index) const { return m_Objects[index]; }

private:
  ezWorld* m_pWorld;
  ezDeque<ezGameObjectHandle> m_Objects;
};

