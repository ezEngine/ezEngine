#pragma once

#include <Core/World/World.h>
#include <Foundation/IO/Stream.h>

class EZ_CORE_DLL ezWorldWriter
{
public:
  void Write(ezStreamWriter& stream, ezWorld& world);

  void WriteHandle(const ezGameObjectHandle& hObject);
  void WriteHandle(const ezComponentHandle& hComponent);

  ezStreamWriter& GetStream() const { return *m_pStream; }

private:
  bool ObjectTraverser(ezGameObject* pObject);
  void WriteGameObject(const ezGameObject* pObject);
  void WriteComponentsOfType(const ezRTTI* pRtti, const ezDeque<const ezComponent*>& components);

  ezStreamWriter* m_pStream;
  ezWorld* m_pWorld;

  ezDeque<const ezGameObject*> m_AllObjects;
  ezHashTable<const ezRTTI*, ezDeque<const ezComponent*>> m_AllComponents;
  ezUInt32 m_uiNumComponents;

  ezMap<ezGameObjectHandle, ezUInt32> m_WrittenGameObjectHandles;
  ezMap<ezComponentHandle, ezUInt32> m_WrittenComponentHandles;
};


