#pragma once

#include <Core/World/World.h>
#include <Foundation/IO/Stream.h>

class EZ_CORE_DLL ezWorldReader
{
public:

  ezStreamReader& GetStream() const { return *m_pStream; }

  void Read(ezStreamReader& stream, ezWorld& world, const ezVec3& vRootPosition = ezVec3(0.0f), const ezQuat& qRootRotation = ezQuat::IdentityQuaternion(), const ezVec3& vRootScale = ezVec3(1.0f));

  ezGameObjectHandle ReadHandle();
  void ReadHandle(ezComponentHandle* out_hComponent);

  ezUInt32 GetComponentTypeVersion(const ezRTTI* pRtti) const;

private:
  ezGameObject* ReadGameObject(bool bRoot);
  void ReadComponentInfo(ezUInt32 uiComponentTypeIdx);
  void ReadComponentsOfType(ezUInt32 uiComponentTypeIdx);
  void FulfillComponentHandleRequets();

  ezStreamReader* m_pStream;
  ezWorld* m_pWorld;

  ezVec3 m_vRootPosition;
  ezQuat m_qRootRotation;
  ezVec3 m_vRootScale;

  ezDynamicArray<ezGameObjectHandle> m_IndexToGameObjectHandle;
  ezDynamicArray<ezComponentHandle> m_IndexToComponentHandle;

  struct CompRequest
  {
    EZ_DECLARE_POD_TYPE();

    ezComponentHandle* m_pWriteToComponent;
    ezUInt32 m_uiComponentIndex;
  };

  ezHybridArray<CompRequest, 64> m_ComponentHandleRequests;
  ezDynamicArray<const ezRTTI*> m_ComponentTypes;
  ezHashTable<const ezRTTI*, ezUInt32> m_ComponentTypeVersions;
};


