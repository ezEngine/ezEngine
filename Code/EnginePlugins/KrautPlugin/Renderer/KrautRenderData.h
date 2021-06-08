#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Foundation/Types/RefCounted.h>
#include <Foundation/Types/SharedPtr.h>
#include <KrautPlugin/KrautDeclarations.h>
#include <RendererCore/Pipeline/RenderData.h>

typedef ezTypedResourceHandle<class ezMeshResource> ezMeshResourceHandle;

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
  ezVec3 m_vWindTrunk = ezVec3::ZeroVector();
  ezVec3 m_vWindBranches = ezVec3::ZeroVector();
};
