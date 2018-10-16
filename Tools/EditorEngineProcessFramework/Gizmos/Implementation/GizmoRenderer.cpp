#include <PCH.h>

#include <EditorEngineProcessFramework/Gizmos/GizmoComponent.h>
#include <EditorEngineProcessFramework/Gizmos/GizmoRenderer.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/Types/ScopeExit.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Pipeline/RenderDataBatch.h>
#include <RendererCore/Pipeline/ViewData.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>

#include <RendererCore/../../../Data/Base/Shaders/Editor/GizmoConstants.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezGizmoRenderer, 1, ezRTTIDefaultAllocator<ezGizmoRenderer>);
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Enabled", m_bEnabled)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_MEMBER_PROPERTY("HighlightID", m_uiHighlightID),
    EZ_MEMBER_PROPERTY("OnlyPickable", m_bOnlyPickable)
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

float ezGizmoRenderer::s_fGizmoScale = 1.0f;

ezGizmoRenderer::ezGizmoRenderer()
{
  m_bEnabled = true;
  m_uiHighlightID = 0;
}

ezGizmoRenderer::~ezGizmoRenderer() {}

void ezGizmoRenderer::GetSupportedRenderDataTypes(ezHybridArray<const ezRTTI*, 8>& types)
{
  types.PushBack(ezGetStaticRTTI<ezGizmoRenderData>());
}

void ezGizmoRenderer::RenderBatch(const ezRenderViewContext& renderViewContext, ezRenderPipelinePass* pPass, const ezRenderDataBatch& batch)
{
  if (!m_bEnabled)
    return;

  const ezGizmoRenderData* pRenderData = batch.GetFirstData<ezGizmoRenderData>();

  const ezMeshResourceHandle& hMesh = pRenderData->m_hMesh;
  const ezMaterialResourceHandle& hMaterial = pRenderData->m_hMaterial;
  ezUInt32 uiSubMeshIndex = pRenderData->m_uiSubMeshIndex;

  ezResourceLock<ezMeshResource> pMesh(hMesh);

  // This can happen when the resource has been reloaded and now has fewer submeshes.
  const auto& subMeshes = pMesh->GetSubMeshes();
  if (subMeshes.GetCount() <= uiSubMeshIndex)
  {
    return;
  }

  const ezMeshResourceDescriptor::SubMesh& meshPart = subMeshes[uiSubMeshIndex];

  renderViewContext.m_pRenderContext->BindMeshBuffer(pMesh->GetMeshBuffer());
  renderViewContext.m_pRenderContext->BindMaterial(hMaterial);

  /// \todo This pattern looks like it is inefficient. Should it use the GPU pool instead somehow?
  ezConstantBufferStorage<ezGizmoConstants>* pGizmoConstantBuffer;
  ezConstantBufferStorageHandle hGizmoConstantBuffer = ezRenderContext::CreateConstantBufferStorage(pGizmoConstantBuffer);
  EZ_SCOPE_EXIT(ezRenderContext::DeleteConstantBufferStorage(hGizmoConstantBuffer));

  renderViewContext.m_pRenderContext->BindConstantBuffer("ezGizmoConstants", hGizmoConstantBuffer);

  // since typically the fov is tied to the height, we orient the gizmo size on that
  const float fGizmoScale = s_fGizmoScale * (128.0f / (float)renderViewContext.m_pViewData->m_ViewPortRect.height);

  for (auto it = batch.GetIterator<ezGizmoRenderData>(); it.IsValid(); ++it)
  {
    pRenderData = it;

    if (m_bOnlyPickable && !pRenderData->m_bIsPickable)
      continue;

    EZ_ASSERT_DEV(pRenderData->m_hMesh == hMesh, "Invalid batching (mesh)");
    EZ_ASSERT_DEV(pRenderData->m_hMaterial == hMaterial, "Invalid batching (material)");
    EZ_ASSERT_DEV(pRenderData->m_uiSubMeshIndex == uiSubMeshIndex, "Invalid batching (part)");

    ezColor color = pRenderData->m_GizmoColor;
    // Highest bit is used to indicate whether the object is dynamic, so exclude it in this check
    if ((pRenderData->m_uiUniqueID & ~(1 << 31)) == m_uiHighlightID)
    {
      color = ezColor(0.9f, 0.9f, 0.1f, color.a);
    }

    ezGizmoConstants& cb = pGizmoConstantBuffer->GetDataForWriting();
    cb.ObjectToWorldMatrix = pRenderData->m_GlobalTransform.GetAsMat4();
    cb.GizmoColor = color;
    cb.GizmoScale = fGizmoScale;
    cb.GameObjectID = pRenderData->m_uiUniqueID;

    if (renderViewContext.m_pRenderContext->DrawMeshBuffer(meshPart.m_uiPrimitiveCount, meshPart.m_uiFirstPrimitive).Failed())
    {
      // draw bounding box instead
      if (pRenderData->m_GlobalBounds.IsValid())
      {
        ezDebugRenderer::DrawLineBox(*renderViewContext.m_pViewDebugContext, pRenderData->m_GlobalBounds.GetBox(), ezColor::Magenta);
      }
    }
  }
}

EZ_STATICLINK_FILE(RendererCore, RendererCore_Meshes_Implementation_MeshRenderer);
