#include <EnginePluginKraut/EnginePluginKrautPCH.h>

#include <EnginePluginKraut/KrautTreeAsset/KrautTreeContext.h>
#include <EnginePluginKraut/KrautTreeAsset/KrautTreeView.h>

#include <GameEngine/Effects/Wind/SimpleWindComponent.h>
#include <KrautPlugin/Resources/KrautGeneratorResource.h>
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

    return;
  }

  ezEngineProcessDocumentContext::HandleMessage(pMsg0);
}

void ezKrautTreeContext::OnInitialize()
{
  auto pWorld = m_pWorld;
  EZ_LOCK(pWorld->GetWriteMarker());


  ezKrautTreeComponent* pTree;

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
  }


  // Wind
  {
    ezGameObjectDesc obj;
    obj.m_sName.Assign("Wind");

    ezGameObject* pObj;
    pWorld->CreateObject(obj, pObj);

    ezSimpleWindComponent* pWind = nullptr;
    ezSimpleWindComponent::CreateComponent(pObj, pWind);

    pWind->m_Deviation = ezAngle::MakeFromDegree(180);
    pWind->m_MinWindStrength = ezWindStrength::Calm;
    pWind->m_MaxWindStrength = ezWindStrength::ModerateBreeze;
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
