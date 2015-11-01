#pragma once

#include <Core/World/World.h>
#include <Foundation/IO/Stream.h>

class EZ_CORE_DLL ezWorldReader
{
public:

  ezStreamReader& GetStream() const { return *m_pStream; }

  void Read(ezStreamReader& stream, ezWorld& world);

  ezGameObjectHandle ReadHandle();
  void ReadHandle(ezComponentHandle* out_hComponent);

private:
  ezGameObject* ReadGameObject();
  void ReadComponentsOfType();
  void FulfillComponentHandleRequets();

  ezStreamReader* m_pStream;
  ezWorld* m_pWorld;

  ezDynamicArray<ezGameObjectHandle> m_IndexToGameObjectHandle;
  ezDynamicArray<ezComponentHandle> m_IndexToComponentHandle;

  struct CompRequest
  {
    EZ_DECLARE_POD_TYPE();

    ezComponentHandle* m_pWriteToComponent;
    ezUInt32 m_uiComponentIndex;
  };

  ezHybridArray<CompRequest, 64> m_ComponentHandleRequests;
};


