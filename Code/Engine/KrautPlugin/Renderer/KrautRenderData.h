#pragma once

#include <KrautPlugin/Basics.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <Core/ResourceManager/ResourceHandle.h>

typedef ezTypedResourceHandle<class ezMeshResource> ezMeshResourceHandle;

class EZ_KRAUTPLUGIN_DLL ezKrautRenderData : public ezRenderData
{
  EZ_ADD_DYNAMIC_REFLECTION(ezKrautRenderData, ezRenderData);

public:
  ezMeshResourceHandle m_hMesh;
  ezUInt32 m_uiSubMeshIndex;
  ezUInt32 m_uiUniqueID;
  float m_fLodDistanceMinSQR;
  float m_fLodDistanceMaxSQR;
};
