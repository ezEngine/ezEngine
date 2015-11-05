#pragma once

#include <Core/World/World.h>
#include <Foundation/IO/Stream.h>
#include <Foundation/Types/TagSet.h>

class EZ_CORE_DLL ezWorldWriter
{
public:
  void Write(ezStreamWriter& stream, ezWorld& world, const ezTagSet* pExclude = nullptr);

  void WriteHandle(const ezGameObjectHandle& hObject);
  void WriteHandle(const ezComponentHandle& hComponent);

  ezStreamWriter& GetStream() const { return *m_pStream; }

private:
  bool ObjectTraverser(ezGameObject* pObject);
  void WriteGameObject(const ezGameObject* pObject);
  void WriteComponentsOfType(const ezRTTI* pRtti, const ezDeque<const ezComponent*>& components);

  ezStreamWriter* m_pStream;
  ezWorld* m_pWorld;
  const ezTagSet* m_pExclude;

  ezDeque<const ezGameObject*> m_AllRootObjects;
  ezDeque<const ezGameObject*> m_AllChildObjects;
  ezHashTable<const ezRTTI*, ezDeque<const ezComponent*>> m_AllComponents;
  ezUInt32 m_uiNumComponents;

  ezMap<ezGameObjectHandle, ezUInt32> m_WrittenGameObjectHandles;
  ezMap<ezComponentHandle, ezUInt32> m_WrittenComponentHandles;
};


