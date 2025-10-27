#pragma once

#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Meshes/MeshResource.h>

class ezRmlUiRenderData : public ezRenderData
{
  EZ_ADD_DYNAMIC_REFLECTION(ezRmlUiRenderData, ezRenderData);

public:
  ezGALTextureHandle m_hTexture;
  ezVec2 m_vOffset = ezVec2::MakeZero();
};


class ezRmlUiMeshRenderData : public ezRenderData
{
  EZ_ADD_DYNAMIC_REFLECTION(ezRmlUiMeshRenderData, ezRenderData);

public:
  void FillSortingKey();
  virtual bool CanBatch(const ezRenderData& other) const override;

  ezGALTextureHandle m_hTexture;
  ezVec2 m_vOffset = ezVec2::MakeZero();

  ezMeshResourceHandle m_hMesh;

  ezUInt32 m_uiSubMeshIndex : 30;
  ezUInt32 m_uiFlipWinding : 1;
  ezUInt32 m_uiUniformScale : 1;

  ezUInt32 m_uiUniqueID = 0;
};
