#pragma once

#include <KrautPlugin/Renderer/KrautRenderData.h>
#include <RendererCore/Pipeline/Renderer.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>

#include <RendererCore/../../../Data/Base/Kraut/TreeShaderData.h>

struct ezPerInstanceData;

/// \brief Implements rendering of static meshes
class EZ_KRAUTPLUGIN_DLL ezKrautRenderer : public ezRenderer
{
  EZ_ADD_DYNAMIC_REFLECTION(ezKrautRenderer, ezRenderer);
  EZ_DISALLOW_COPY_AND_ASSIGN(ezKrautRenderer);

public:
  ezKrautRenderer();
  ~ezKrautRenderer();

  // ezRenderer implementation
  virtual void GetSupportedRenderDataTypes(ezHybridArray<const ezRTTI*, 8>& types) const override;
  virtual void GetSupportedRenderDataCategories(ezHybridArray<ezRenderData::Category, 8>& categories) const override;
  virtual void RenderBatch(
    const ezRenderViewContext& renderContext, const ezRenderPipelinePass* pPass, const ezRenderDataBatch& batch) const override;

protected:
  virtual void FillPerInstanceData(const ezVec3& vLodCamPos, ezArrayPtr<ezPerInstanceData> instanceData, const ezRenderDataBatch& batch,
    bool bUpdateMinLod, bool bIsShadowView, ezUInt32 uiStartIndex, ezUInt32& out_uiFilteredCount) const;

  struct TempTreeCB
  {
    TempTreeCB(ezRenderContext* pRenderContext);
    ~TempTreeCB();

    void SetTreeData(const ezVec3& vTreeCenter, float fLeafShadowOffset);

    ezConstantBufferStorage<ezKrautTreeConstants>* m_pConstants;
    ezConstantBufferStorageHandle m_hConstantBuffer;
  };
};
