#include <EnginePluginKraut/EnginePluginKrautPCH.h>

#include <EnginePluginKraut/KrautTreeAsset/KrautTreeContext.h>
#include <EnginePluginKraut/KrautTreeAsset/KrautTreeView.h>

#include <GameEngine/Effects/Wind/SimpleWindComponent.h>
#include <KrautPlugin/Resources/KrautGeneratorResource.h>
#include <KrautPlugin/Resources/KrautTreeResource.h>
#include <RendererCore/Components/SkyBoxComponent.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezKrautTreeContext, 1, ezRTTIDefaultAllocator<ezKrautTreeContext>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_CONSTANT_PROPERTY("DocumentType", (const char*) "Kraut Tree"),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezKrautTreeContext::ezKrautTreeContext()
  : ezEngineProcessDocumentContext(ezEngineProcessDocumentContextFlags::CreateWorld)
{
  m_pMainObject = nullptr;
}

void ezKrautTreeContext::HandleMessage(const ezEditorEngineDocumentMsg* pMsg0)
{
  if (auto pMsg = ezDynamicCast<const ezQuerySelectionBBoxMsgToEngine*>(pMsg0))
  {
    QuerySelectionBBox(pMsg);
    return;
  }

  if (auto pMsg = ezDynamicCast<const ezSimpleDocumentConfigMsgToEngine*>(pMsg0))
  {
    if (pMsg->m_sWhatToDo == "UpdateTree" && !m_hKrautComponent.IsInvalidated())
    {
      EZ_LOCK(m_pWorld->GetWriteMarker());

      ezKrautTreeComponent* pTree = nullptr;
      if (!m_pWorld->TryGetComponent(m_hKrautComponent, pTree))
        return;

      if (pMsg->m_sPayload == "DisplayRandomSeed")
      {
        m_uiDisplayRandomSeed = pMsg->m_PayloadValue.ConvertTo<ezUInt32>();

        pTree->SetCustomRandomSeed(m_uiDisplayRandomSeed);
      }
    }

    if (pMsg->m_sWhatToDo == "SetWindStrength" && !m_hWindComponent.IsInvalidated())
    {
      EZ_LOCK(m_pWorld->GetWriteMarker());

      ezSimpleWindComponent* pWind = nullptr;
      if (!m_pWorld->TryGetComponent(m_hWindComponent, pWind))
        return;

      const ezInt32 windStrength = pMsg->m_PayloadValue.ConvertTo<ezInt32>();

      switch (windStrength)
      {
        case 0: // Off
          pWind->m_MinWindStrength = ezWindStrength::None;
          pWind->m_MaxWindStrength = ezWindStrength::None;
          break;

        case 1: // Light
          pWind->m_MinWindStrength = ezWindStrength::Calm;
          pWind->m_MaxWindStrength = ezWindStrength::GentleBreeze;
          break;

        case 2: // Moderate
          pWind->m_MinWindStrength = ezWindStrength::GentleBreeze;
          pWind->m_MaxWindStrength = ezWindStrength::StrongBreeze;
          break;

        case 3: // Strong
          pWind->m_MinWindStrength = ezWindStrength::Storm;
          pWind->m_MaxWindStrength = ezWindStrength::Storm;
          break;
      }
    }

    if (pMsg->m_sWhatToDo == "SetLodOverride" && !m_hKrautComponent.IsInvalidated())
    {
      EZ_LOCK(m_pWorld->GetWriteMarker());

      ezKrautTreeComponent* pTree = nullptr;
      if (!m_pWorld->TryGetComponent(m_hKrautComponent, pTree))
        return;

      const ezInt8 iLodOverride = static_cast<ezInt8>(pMsg->m_PayloadValue.ConvertTo<ezInt32>());
      pTree->m_iLodOverride = iLodOverride;
    }

    if (pMsg->m_sWhatToDo == "SetShowFrondsLeaves" && !m_hKrautComponent.IsInvalidated())
    {
      EZ_LOCK(m_pWorld->GetWriteMarker());

      ezKrautTreeComponent* pTree = nullptr;
      if (!m_pWorld->TryGetComponent(m_hKrautComponent, pTree))
        return;

      pTree->m_bHideFrondsAndLeafs = !pMsg->m_PayloadValue.ConvertTo<bool>();
    }

    return;
  }

  if (auto pMsg = ezDynamicCast<const ezViewRedrawMsgToEngine*>(pMsg0))
  {
    SendLodStats(pMsg->m_DocumentGuid);
  }

  ezEngineProcessDocumentContext::HandleMessage(pMsg0);
}

void ezKrautTreeContext::SendLodStats(const ezUuid& documentGuid)
{
  if (m_hKrautComponent.IsInvalidated())
    return;

  EZ_LOCK(m_pWorld->GetWriteMarker());

  ezKrautTreeComponent* pTree = nullptr;
  if (!m_pWorld->TryGetComponent(m_hKrautComponent, pTree))
    return;

  if (!pTree->GetKrautTreeResource().IsValid())
    return;

  ezResourceLock<ezKrautTreeResource> pResource(pTree->GetKrautTreeResource(), ezResourceAcquireMode::AllowLoadingFallback);
  if (pResource.GetAcquireResult() != ezResourceAcquireResult::Final)
    return;

  const auto treeLods = pResource->GetTreeLODs();
  const ezInt8 iLodIdx = pTree->m_iLodOverride;

  if (iLodIdx < 0 || iLodIdx >= static_cast<ezInt8>(treeLods.GetCount()))
    return;

  const auto& lod = treeLods[iLodIdx];

  LodStats stats;
  stats.m_iLodIndex = iLodIdx;
  stats.m_uiNumBones = lod.m_uiNumBones;
  stats.m_uiNumTrianglesTotal = lod.m_uiNumTrianglesBranch + lod.m_uiNumTrianglesFrond + lod.m_uiNumTrianglesLeaf;
  stats.m_uiNumTrianglesBranch = lod.m_uiNumTrianglesBranch;
  stats.m_uiNumTrianglesFrond = lod.m_uiNumTrianglesFrond;
  stats.m_uiNumTrianglesLeaf = lod.m_uiNumTrianglesLeaf;

  if (stats.m_iLodIndex == m_LastSentLodStats.m_iLodIndex &&
      stats.m_uiNumBones == m_LastSentLodStats.m_uiNumBones &&
      stats.m_uiNumTrianglesTotal == m_LastSentLodStats.m_uiNumTrianglesTotal &&
      stats.m_uiNumTrianglesBranch == m_LastSentLodStats.m_uiNumTrianglesBranch &&
      stats.m_uiNumTrianglesFrond == m_LastSentLodStats.m_uiNumTrianglesFrond &&
      stats.m_uiNumTrianglesLeaf == m_LastSentLodStats.m_uiNumTrianglesLeaf)
    return;

  m_LastSentLodStats = stats;

  ezStringBuilder sPayload;
  sPayload.SetFormat("{};{};{};{};{}",
    stats.m_uiNumBones,
    stats.m_uiNumTrianglesTotal,
    stats.m_uiNumTrianglesBranch,
    stats.m_uiNumTrianglesFrond,
    stats.m_uiNumTrianglesLeaf);

  ezSimpleDocumentConfigMsgToEditor msg;
  msg.m_DocumentGuid = documentGuid;
  msg.m_sWhatToDo = "LODStats";
  msg.m_sPayload = sPayload;
  SendProcessMessage(&msg);
}

void ezKrautTreeContext::OnInitialize()
{
  auto pWorld = m_pWorld;
  EZ_LOCK(pWorld->GetWriteMarker());


  ezKrautTreeComponent* pTree = nullptr;

  // Preview Mesh
  {
    ezGameObjectDesc obj;
    obj.m_sName.Assign("KrautTreePreview");
    // TODO: making the object dynamic is a workaround!
    // without it, shadows keep disappearing when switching between tree documents
    // triggering resource reload will also trigger ezKrautTreeComponent::OnMsgExtractRenderData,
    // which fixes the shadows for a while, but not caching the render-data (ezRenderData::Caching::IfStatic)
    // 'solves' the issue in the preview
    obj.m_bDynamic = true;
    pWorld->CreateObject(obj, m_pMainObject);

    const ezTag& tagCastShadows = ezTagRegistry::GetGlobalRegistry().RegisterTag("CastShadow");
    m_pMainObject->SetTag(tagCastShadows);

    m_hKrautComponent = ezKrautTreeComponent::CreateComponent(m_pMainObject, pTree);
    ezStringBuilder sMeshGuid;
    ezConversionUtils::ToString(GetDocumentGuid(), sMeshGuid);
    m_hMainResource = ezResourceManager::LoadResource<ezKrautGeneratorResource>(sMeshGuid);
    pTree->SetVariationIndex(0xFFFF); // takes the 'display seed'
    pTree->SetKrautGeneratorResource(m_hMainResource);
    pTree->m_bForceGenerateImmediate = true;
  }


  // Wind
  {
    ezGameObjectDesc obj;
    obj.m_sName.Assign("Wind");

    ezGameObject* pObj;
    pWorld->CreateObject(obj, pObj);

    ezSimpleWindComponent* pWind = nullptr;
    m_hWindComponent = ezSimpleWindComponent::CreateComponent(pObj, pWind);

    pWind->m_Deviation = ezAngle::MakeFromDegree(180);
    pWind->m_MinWindStrength = ezWindStrength::LightBreeze;
    pWind->m_MaxWindStrength = ezWindStrength::GentleBreeze;
  }

  // ground
  {
    const char* szMeshName = "KrautPreviewGroundMesh";
    m_hPreviewMeshResource = ezResourceManager::GetExistingResource<ezMeshResource>(szMeshName);

    if (!m_hPreviewMeshResource.IsValid())
    {
      const char* szMeshBufferName = "KrautPreviewGroundMeshBuffer";

      ezMeshBufferResourceHandle hMeshBuffer = ezResourceManager::GetExistingResource<ezMeshBufferResource>(szMeshBufferName);

      if (!hMeshBuffer.IsValid())
      {
        // Build geometry
        ezGeometry::GeoOptions opt;
        opt.m_Transform = ezMat4::MakeTranslation(ezVec3(0, 0, -0.05f));

        ezGeometry geom;
        geom.AddCylinder(8.0f, 7.9f, 0.05f, 0.05f, true, true, 32, opt);
        geom.TriangulatePolygons();
        geom.ComputeTangents();

        ezMeshBufferResourceDescriptor desc;
        desc.AddCommonStreams();
        desc.AllocateStreamsFromGeometry(geom, ezGALPrimitiveTopology::Triangles);

        hMeshBuffer = ezResourceManager::GetOrCreateResource<ezMeshBufferResource>(szMeshBufferName, std::move(desc), szMeshBufferName);
      }
      {
        ezResourceLock<ezMeshBufferResource> pMeshBuffer(hMeshBuffer, ezResourceAcquireMode::AllowLoadingFallback);

        ezMeshResourceDescriptor md;
        md.UseExistingMeshBuffer(hMeshBuffer);
        md.AddSubMesh(pMeshBuffer->GetPrimitiveCount(), 0, 0);
        md.SetMaterial(0, "{ 1c47ee4c-0379-4280-85f5-b8cda61941d2 }"); // Pattern.ezMaterialAsset
        md.ComputeBounds();

        m_hPreviewMeshResource = ezResourceManager::GetOrCreateResource<ezMeshResource>(szMeshName, std::move(md), pMeshBuffer->GetResourceDescription());
      }
    }

    // Ground Mesh Component
    {
      ezGameObjectDesc obj;
      obj.m_sName.Assign("KrautGround");

      ezGameObject* pObj;
      pWorld->CreateObject(obj, pObj);

      ezMeshComponent* pMesh;
      ezMeshComponent::CreateComponent(pObj, pMesh);
      pMesh->SetMesh(m_hPreviewMeshResource);
    }
  }
}

ezEngineProcessViewContext* ezKrautTreeContext::CreateViewContext()
{
  return EZ_DEFAULT_NEW(ezKrautTreeViewContext, this);
}

void ezKrautTreeContext::DestroyViewContext(ezEngineProcessViewContext* pContext)
{
  EZ_DEFAULT_DELETE(pContext);
}

bool ezKrautTreeContext::UpdateThumbnailViewContext(ezEngineProcessViewContext* pThumbnailViewContext)
{
  {
    EZ_LOCK(m_pWorld->GetWriteMarker());

    m_pMainObject->UpdateLocalBounds();
    m_pMainObject->UpdateGlobalTransformAndBounds();
  }

  ezBoundingBoxSphere bounds = m_pMainObject->GetGlobalBounds();

  // Bounds are only valid once the base-data task has run and SetDetails() was called.
  // Until then return false so the convergence counter doesn't start and the camera isn't
  // focused on an invalid bounding sphere.
  if (!bounds.IsValid())
    return false;

  // undo the artificial bounds scale to get a tight bbox for better thumbnails
  const float fAdditionalZoom = 1.5f;
  bounds.m_fSphereRadius /= ezKrautTreeComponent::s_iLocalBoundsScale * fAdditionalZoom;
  bounds.m_vBoxHalfExtents /= ezKrautTreeComponent::s_iLocalBoundsScale * fAdditionalZoom;

  ezKrautTreeViewContext* pMeshViewContext = static_cast<ezKrautTreeViewContext*>(pThumbnailViewContext);
  return pMeshViewContext->UpdateThumbnailCamera(bounds);
}


void ezKrautTreeContext::QuerySelectionBBox(const ezEditorEngineDocumentMsg* pMsg)
{
  if (m_pMainObject == nullptr)
    return;

  ezBoundingBoxSphere bounds = ezBoundingBoxSphere::MakeInvalid();

  {
    EZ_LOCK(m_pWorld->GetWriteMarker());

    m_pMainObject->UpdateLocalBounds();
    m_pMainObject->UpdateGlobalTransformAndBounds();
    auto b = m_pMainObject->GetGlobalBounds();

    if (b.IsValid())
    {
      b.m_fSphereRadius /= ezKrautTreeComponent::s_iLocalBoundsScale;
      b.m_vBoxHalfExtents /= (float)ezKrautTreeComponent::s_iLocalBoundsScale;

      bounds.ExpandToInclude(b);
    }
  }

  const ezQuerySelectionBBoxMsgToEngine* msg = static_cast<const ezQuerySelectionBBoxMsgToEngine*>(pMsg);

  ezQuerySelectionBBoxResultMsgToEditor res;
  res.m_uiViewID = msg->m_uiViewID;
  res.m_iPurpose = msg->m_iPurpose;
  res.m_vCenter = bounds.m_vCenter;
  res.m_vHalfExtents = bounds.m_vBoxHalfExtents;
  res.m_DocumentGuid = pMsg->m_DocumentGuid;

  SendProcessMessage(&res);
}
