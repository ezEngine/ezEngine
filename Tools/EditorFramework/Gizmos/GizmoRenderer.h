#pragma once

#include <EditorFramework/Plugin.h>
#include <RendererCore/Pipeline/Declarations.h>
#include <Core/ResourceManager/ResourceHandle.h>

typedef ezTypedResourceHandle<class ezConstantBufferResource> ezConstantBufferResourceHandle;

class EZ_EDITORFRAMEWORK_DLL ezGizmoRenderer : public ezRenderer
{
  EZ_ADD_DYNAMIC_REFLECTION(ezGizmoRenderer, ezRenderer);

public:
  ezGizmoRenderer();
  ~ezGizmoRenderer();

  // ezRenderer implementation
  virtual void GetSupportedRenderDataTypes(ezHybridArray<const ezRTTI*, 8>& types) override;
  virtual void RenderBatch(const ezRenderViewContext& renderContext, ezRenderPipelinePass* pPass, const ezRenderDataBatch& batch) override;

  static float s_fGizmoScale;

private:

  ezUInt32 m_uiHighlightID;

  ezConstantBufferResourceHandle m_hGizmoConstantBuffer;
};

