#include <EnginePluginAssets/EnginePluginAssetsPCH.h>

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>
#include <EnginePluginAssets/MeshAsset/MeshContext.h>
#include <EnginePluginAssets/MeshAsset/MeshView.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererFoundation/Resources/Buffer.h>

ezMeshViewContext::ezMeshViewContext(ezMeshContext* pMeshContext)
  : ezEngineProcessViewContext(pMeshContext)
{
  m_pContext = pMeshContext;

  // Start with something valid.
  m_Camera.SetCameraMode(ezCameraMode::PerspectiveFixedFovX, 45.0f, 0.05f, 10000.0f);
  m_Camera.LookAt(ezVec3(1, 1, 1), ezVec3::MakeZero(), ezVec3(0.0f, 0.0f, 1.0f));
}

ezMeshViewContext::~ezMeshViewContext() = default;

bool ezMeshViewContext::UpdateThumbnailCamera(const ezBoundingBoxSphere& bounds)
{
  return !FocusCameraOnObject(m_Camera, bounds, 45.0f, -ezVec3(5, -2, 3));
}


ezViewHandle ezMeshViewContext::CreateView()
{
  ezView* pView = nullptr;
  ezRenderWorld::CreateView("Mesh Editor - View", pView);
  pView->SetCameraUsageHint(ezCameraUsageHint::EditorView);

  pView->SetRenderPipelineResource(CreateDefaultRenderPipeline());

  ezEngineProcessDocumentContext* pDocumentContext = GetDocumentContext();
  pView->SetWorld(pDocumentContext->GetWorld());
  pView->SetCamera(&m_Camera);
  return pView->GetHandle();
}

void ezMeshViewContext::SetCamera(const ezViewRedrawMsgToEngine* pMsg)
{
  if (m_pContext->m_bDisplayGrid)
  {
    ezEngineProcessViewContext::DrawSimpleGrid();
  }

  ezEngineProcessViewContext::SetCamera(pMsg);

  auto hMesh = m_pContext->GetMesh();
  if (hMesh.IsValid())
  {
    ezResourceLock<ezMeshResource> pMesh(hMesh, ezResourceAcquireMode::AllowLoadingFallback);
    ezResourceLock<ezMeshBufferResource> pMeshBuffer(pMesh->GetMeshBuffer(), ezResourceAcquireMode::AllowLoadingFallback);

    ezUInt32 uiNumVertices = 0;
    ezUInt32 uiVertexByteSize = 0;
    for (auto hBuffer : pMeshBuffer->GetVertexBuffers())
    {
      if (auto pBuffer = ezGALDevice::GetDefaultDevice()->GetBuffer(hBuffer))
      {
        auto& bufferDesc = pBuffer->GetDescription();
        uiNumVertices = ezMath::Max(uiNumVertices, bufferDesc.m_uiTotalSize / bufferDesc.m_uiStructSize);
        uiVertexByteSize += bufferDesc.m_uiStructSize;
      }
    }

    const ezUInt32 uiNumTriangles = pMeshBuffer->GetPrimitiveCount();
    ezVec3 bboxExtents = ezVec3(2);

    if (pMeshBuffer->GetBounds().IsValid())
    {
      bboxExtents = pMeshBuffer->GetBounds().m_vBoxHalfExtents * 2.0f;
    }

    auto& streamConfig = pMeshBuffer->GetVertexStreamConfig();
    const ezUInt32 uiNumUVs = streamConfig.HasTexCoord0() + streamConfig.HasTexCoord1();
    const ezUInt32 uiNumColors = streamConfig.HasColor0() + streamConfig.HasColor1();

    ezStringBuilder sText;
    sText.AppendFormat("Triangles: \t{}\t\n", uiNumTriangles);
    sText.AppendFormat("Vertices: \t{}\t\n", uiNumVertices);
    sText.AppendFormat("UV Channels: \t{}\t\n", uiNumUVs);
    sText.AppendFormat("Color Channels: \t{}\t\n", uiNumColors);
    sText.AppendFormat("Bytes Per Vertex: \t{}\t\n", uiVertexByteSize);
    sText.AppendFormat("Bounding Box: \twidth={0}, depth={1}, height={2}\t", ezArgF(bboxExtents.x, 2), ezArgF(bboxExtents.y, 2), ezArgF(bboxExtents.z, 2));

    ezDebugRenderer::DrawInfoText(m_hView, ezDebugTextPlacement::BottomLeft, "AssetStats", sText);
  }

  if (m_uiLastHoveredPartIndex != ezInvalidIndex && m_uiLastHoveredPartIndex < (ezUInt32)m_pContext->m_SlotNames.GetCount())
  {
    ezStringBuilder sHoverText;
    sHoverText.AppendFormat("Slot {}: {}\t\n(Ctrl+MMB to open)\t", m_uiLastHoveredPartIndex, m_pContext->m_SlotNames[m_uiLastHoveredPartIndex]);
    ezDebugRenderer::DrawInfoText(m_hView, ezDebugTextPlacement::TopLeft, "HoveredMaterial", sHoverText);
  }
}

void ezMeshViewContext::HandleViewMessage(const ezEditorEngineViewMsg* pMsg)
{
  ezEngineProcessViewContext::HandleViewMessage(pMsg);

  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<ezViewRedrawMsgToEngine>())
  {
    const ezViewRedrawMsgToEngine* pMsg2 = static_cast<const ezViewRedrawMsgToEngine*>(pMsg);

    ezView* pView = nullptr;
    if (ezRenderWorld::TryGetView(m_hView, pView))
    {
      pView->SetRenderPassProperty("EditorPickingPass", "Active", pMsg2->m_bUpdatePickingData);
      pView->SetRenderPassProperty("EditorPickingPass", "PickSelected", pMsg2->m_bEnablePickingSelected);
      pView->SetRenderPassProperty("EditorPickingPass", "PickTransparent", pMsg2->m_bEnablePickTransparent);
    }
  }
  else if (pMsg->GetDynamicRTTI()->IsDerivedFrom<ezViewPickingMsgToEngine>())
  {
    const ezViewPickingMsgToEngine* pMsg2 = static_cast<const ezViewPickingMsgToEngine*>(pMsg);
    PickObjectAt(pMsg2->m_uiPickPosX, pMsg2->m_uiPickPosY);
  }
}

void ezMeshViewContext::PickObjectAt(ezUInt16 x, ezUInt16 y)
{
  ezViewPickingResultMsgToEditor res;
  EZ_SCOPE_EXIT(SendViewMessage(&res));

  ezView* pView = nullptr;
  if (!ezRenderWorld::TryGetView(m_hView, pView))
    return;

  pView->SetRenderPassProperty("EditorPickingPass", "PickingPosition", ezVec2(x, y));

  if (!pView->IsRenderPassReadBackPropertyExisting("EditorPickingPass", "PickedPosition"))
    return;

  ezVariant varPickedPos = pView->GetRenderPassReadBackProperty("EditorPickingPass", "PickedPosition");
  if (!varPickedPos.IsA<ezVec3>())
    return;

  const ezUInt32 uiPickingID = pView->GetRenderPassReadBackProperty("EditorPickingPass", "PickedID").ConvertTo<ezUInt32>();
  res.m_vPickedNormal = pView->GetRenderPassReadBackProperty("EditorPickingPass", "PickedNormal").ConvertTo<ezVec3>();
  res.m_vPickingRayStartPosition = pView->GetRenderPassReadBackProperty("EditorPickingPass", "PickedRayStartPosition").ConvertTo<ezVec3>();
  res.m_vPickedPosition = varPickedPos.ConvertTo<ezVec3>();

  // The component and object GUIDs are not available in the mesh preview since the objects
  // are not registered in the component picking map. Only the part index (material slot) is relevant here.
  res.m_uiPartIndex = (uiPickingID >> 24) & 0xFF;

  m_uiLastHoveredPartIndex = (uiPickingID != 0) ? res.m_uiPartIndex : ezInvalidIndex;
}
