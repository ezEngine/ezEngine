#include <PCH.h>
#include <EditorFramework/Gizmos/GizmoRenderer.h> 
#include <EditorFramework/Gizmos/GizmoComponent.h> 

#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Pipeline/RenderDataBatch.h>
#include <RendererCore/Pipeline/ViewData.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>
#include <Core/ResourceManager/ResourceManager.h>

#include <RendererCore/../../../Data/Base/Shaders/Editor/GizmoConstants.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezGizmoRenderer, 1, ezRTTIDefaultAllocator<ezGizmoRenderer>);
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("HighlightID", m_uiHighlightID)
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

float ezGizmoRenderer::s_fGizmoScale = 1.0f;

ezGizmoRenderer::ezGizmoRenderer()
{
  m_uiHighlightID = 0;
  m_bEnabled = true;
}

ezGizmoRenderer::~ezGizmoRenderer()
{
}

void ezGizmoRenderer::GetSupportedRenderDataTypes(ezHybridArray<const ezRTTI*, 8>& types)
{
  types.PushBack(ezGetStaticRTTI<ezGizmoRenderData>());
}

void ezGizmoRenderer::RenderBatch(const ezRenderViewContext& renderViewContext, ezRenderPipelinePass* pPass, const ezRenderDataBatch& batch)
{
  if (!m_bEnabled)
    return;

  const ezGizmoRenderData* pRenderData = batch.GetData<ezGizmoRenderData>(0);

  const ezMeshResourceHandle& hMesh = pRenderData->m_hMesh;
  const ezMaterialResourceHandle& hMaterial = pRenderData->m_hMaterial;
  ezUInt32 uiPartIndex = pRenderData->m_uiPartIndex;

  ezResourceLock<ezMeshResource> pMesh(hMesh);

  // This can happen when the resource has been reloaded and now has fewer submeshes.
  const auto& subMeshes = pMesh->GetSubMeshes();
  if (subMeshes.GetCount() <= uiPartIndex)
  {
    return;
  }

  const ezMeshResourceDescriptor::SubMesh& meshPart = subMeshes[uiPartIndex];

  renderViewContext.m_pRenderContext->BindMeshBuffer(pMesh->GetMeshBuffer());
  renderViewContext.m_pRenderContext->BindMaterial(hMaterial);  

  ezConstantBufferStorage<GizmoConstants>* pGizmoConstantBuffer;
  ezConstantBufferStorageHandle hGizmoConstantBuffer = ezRenderContext::CreateConstantBufferStorage(pGizmoConstantBuffer);

  renderViewContext.m_pRenderContext->BindConstantBuffer("GizmoConstants", hGizmoConstantBuffer);

  // since typically the fov is tied to the height, we orient the gizmo size on that
  const float fGizmoScale = s_fGizmoScale * (128.0f / (float)renderViewContext.m_pViewData->m_ViewPortRect.height);

  for (auto it = batch.GetIterator<ezGizmoRenderData>(); it.IsValid(); ++it)
  {
    pRenderData = it;

    EZ_ASSERT_DEV(pRenderData->m_hMesh == hMesh, "Invalid batching (mesh)");
    EZ_ASSERT_DEV(pRenderData->m_hMaterial == hMaterial, "Invalid batching (material)");
    EZ_ASSERT_DEV(pRenderData->m_uiPartIndex == uiPartIndex, "Invalid batching (part)");

    ezColor color = pRenderData->m_GizmoColor;
    if (pRenderData->m_uiEditorPickingID == m_uiHighlightID)
    {
      ezColor highlight(0.1f, 0.1f, 0.1f);
      color = color * 2.0f + highlight;
    }

    GizmoConstants& cb = pGizmoConstantBuffer->GetDataForWriting();
    cb.ObjectToWorldMatrix = pRenderData->m_GlobalTransform.GetAsMat4();
    cb.GizmoColor = color;
    cb.GizmoScale = fGizmoScale;
    cb.GameObjectID = pRenderData->m_uiEditorPickingID;

    if (renderViewContext.m_pRenderContext->DrawMeshBuffer(meshPart.m_uiPrimitiveCount, meshPart.m_uiFirstPrimitive).Failed())
    {
      // draw bounding box instead
      if (pRenderData->m_GlobalBounds.IsValid())
      {
        ezDebugRenderer::DrawLineBox(renderViewContext.m_uiWorldIndex, pRenderData->m_GlobalBounds.GetBox(), ezColor::Magenta);
      }
    }
  }

  ezRenderContext::DeleteConstantBufferStorage(hGizmoConstantBuffer);
}

EZ_STATICLINK_FILE(RendererCore, RendererCore_Meshes_Implementation_MeshRenderer);

