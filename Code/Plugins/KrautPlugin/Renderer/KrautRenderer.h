#pragma once

#include <KrautPlugin/Renderer/KrautRenderData.h>
#include <RendererCore/Pipeline/Declarations.h>
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
  virtual void GetSupportedRenderDataTypes(ezHybridArray<const ezRTTI*, 8>& types) override;
  virtual void RenderBatch(const ezRenderViewContext& renderContext, ezRenderPipelinePass* pPass, const ezRenderDataBatch& batch) override;

protected:
  virtual void FillPerInstanceData(const ezVec3& vLodCamPos, ezArrayPtr<ezPerInstanceData> instanceData,
    const ezRenderDataBatch& batch, bool bUpdateMinLod, bool bIsShadowView, ezUInt32 uiStartIndex, ezUInt32& out_uiFilteredCount);

  struct TempTreeCB
  {
    TempTreeCB(ezRenderContext* pRenderContext);
    ~TempTreeCB();

    void SetTreeData(const ezVec3& vTreeCenter, float fLeafShadowOffset);

    ezConstantBufferStorage<ezKrautTreeConstants>* m_pConstants;
    ezConstantBufferStorageHandle m_hConstantBuffer;
  };
};
