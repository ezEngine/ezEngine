#pragma once

#include <RmlUiPlugin/RmlUiPluginDLL.h>

#include <Core/ResourceManager/ResourceHandle.h>
#include <RendererCore/Pipeline/Renderer.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>

class ezRmlUiMeshRenderData;
struct ezPerInstanceData;

using ezShaderResourceHandle = ezTypedResourceHandle<class ezShaderResource>;

class EZ_RMLUIPLUGIN_DLL ezRmlUiMeshRenderer : public ezRenderer
{
  EZ_ADD_DYNAMIC_REFLECTION(ezRmlUiMeshRenderer, ezRenderer);
  EZ_DISALLOW_COPY_AND_ASSIGN(ezRmlUiMeshRenderer);

public:
  ezRmlUiMeshRenderer();
  ~ezRmlUiMeshRenderer();

  // ezRenderer implementation
  virtual void GetSupportedRenderDataTypes(ezHybridArray<const ezRTTI*, 8>& ref_types) const override;
  virtual void GetSupportedRenderDataCategories(ezHybridArray<ezRenderData::Category, 8>& ref_categories) const override;

  virtual void RenderBatch(const ezRenderViewContext& renderViewContext, const ezRenderPipelinePass* pPass, const ezRenderDataBatch& batch) const override;

protected:
  virtual void SetAdditionalData(const ezRenderViewContext& renderViewContext, const ezRmlUiMeshRenderData* pRenderData) const;
  virtual void FillPerInstanceData(
    ezArrayPtr<ezPerInstanceData> instanceData, const ezRenderDataBatch& batch, ezUInt32 uiStartIndex, ezUInt32& out_uiFilteredCount) const;

private:
  ezShaderResourceHandle m_hShader;
  //ezConstantBufferStorageHandle m_hConstantBuffer;
};
