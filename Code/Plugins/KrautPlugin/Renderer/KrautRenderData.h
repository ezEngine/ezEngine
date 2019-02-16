#pragma once

#include <KrautPlugin/KrautDeclarations.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <Core/ResourceManager/ResourceHandle.h>
#include <Foundation/Types/RefCounted.h>
#include <Foundation/Types/SharedPtr.h>

typedef ezTypedResourceHandle<class ezMeshResource> ezMeshResourceHandle;

/// \brief Stores the last LOD state for an object that is made up of multiple parts
class ezKrautLodInfo : public ezRefCounted
{
public:
  // we need 3 separate values for threaded and delayed access:
  //   1. one value to read the last state
  //   2. one value to write the current state (min of own value and existing one)
  //   3. one to clear to a start state (starting value for 2.)
  // which array index is used is determined through the current frame counter % 4
  // the fourth value is a dummy so that we don't need to do modulo 3, which is much slower than modulo 4
  ezUInt32 m_uiMinLod[4] = {16, 16, 16, 16};
};

class EZ_KRAUTPLUGIN_DLL ezKrautRenderData : public ezRenderData
{
  EZ_ADD_DYNAMIC_REFLECTION(ezKrautRenderData, ezRenderData);

public:
  ezMeshResourceHandle m_hMesh;
  ezUInt32 m_uiUniqueID = 0;
  float m_fLodDistanceMinSQR;
  float m_fLodDistanceMaxSQR;
  ezVec3 m_vLeafCenter;

  ezUInt8 m_uiSubMeshIndex = 0;
  ezUInt8 m_uiThisLodIndex = 0;
  bool m_bCastShadows = false;


  ezSharedPtr<ezKrautLodInfo> m_pTreeLodInfo;
};
