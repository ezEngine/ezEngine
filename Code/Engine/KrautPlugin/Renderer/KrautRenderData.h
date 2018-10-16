#pragma once

#include <KrautPlugin/Basics.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <Core/ResourceManager/ResourceHandle.h>

typedef ezTypedResourceHandle<class ezMeshResource> ezMeshResourceHandle;

class EZ_KRAUTPLUGIN_DLL ezKrautBranchRenderData : public ezRenderData
{
  EZ_ADD_DYNAMIC_REFLECTION(ezKrautBranchRenderData, ezRenderData);

public:
  ezMeshResourceHandle m_hMesh;
  ezUInt32 m_uiPartIndex;
  ezUInt32 m_uiUniqueID;
  float m_fLodDistanceMinSQR;
  float m_fLodDistanceMaxSQR;
};

//class EZ_KRAUTPLUGIN_DLL ezKrautFrondsRenderData : public ezRenderData
//{
//  EZ_ADD_DYNAMIC_REFLECTION(ezKrautFrondsRenderData, ezRenderData);
//
//public:
//
//
//};
//
//class EZ_KRAUTPLUGIN_DLL ezKrautLeavesRenderData : public ezRenderData
//{
//  EZ_ADD_DYNAMIC_REFLECTION(ezKrautLeavesRenderData, ezRenderData);
//
//public:
//};
