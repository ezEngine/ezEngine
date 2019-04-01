#include <KrautPluginPCH.h>

#include <Core/Graphics/Geometry.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Core/Utils/WorldGeoExtractionUtil.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Interfaces/PhysicsWorldModule.h>
#include <KrautPlugin/Components/KrautTreeComponent.h>
#include <KrautPlugin/Renderer/KrautRenderData.h>
#include <KrautPlugin/Resources/KrautTreeResource.h>
#include <RendererCore/Meshes/MeshComponentBase.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezKrautTreeComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("KrautTree", GetKrautTreeFile, SetKrautTreeFile)->AddAttributes(new ezAssetBrowserAttribute("Kraut Tree")),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgExtractRenderData, OnExtractRenderData),
    EZ_MESSAGE_HANDLER(ezMsgExtractGeometry, OnExtractGeometry),
    EZ_MESSAGE_HANDLER(ezMsgBuildStaticMesh, OnBuildStaticMesh),
  }
  EZ_END_MESSAGEHANDLERS;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Vegetation"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezKrautTreeComponent::ezKrautTreeComponent()
{
  m_pLodInfo = EZ_DEFAULT_NEW(ezKrautLodInfo);
}

ezKrautTreeComponent::~ezKrautTreeComponent() = default;

void ezKrautTreeComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  auto& s = stream.GetStream();

  s << m_hKrautTree;
}


void ezKrautTreeComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = stream.GetStream();

  s >> m_hKrautTree;

  GetWorld()->GetOrCreateComponentManager<ezKrautTreeComponentManager>()->EnqueueUpdate(GetHandle());
}

ezResult ezKrautTreeComponent::GetLocalBounds(ezBoundingBoxSphere& bounds, bool& bAlwaysVisible)
{
  if (m_hKrautTree.IsValid())
  {
    ezResourceLock<ezKrautTreeResource> pTree(m_hKrautTree, ezResourceAcquireMode::AllowFallback);
    // TODO: handle fallback case properly

    bounds = pTree->GetDetails().m_Bounds;

    {
      // this is a work around to make shadows and LODing work better
      // shadows do not affect the maximum LOD of a tree that is being rendered,
      // otherwise moving/rotating light-sources would case LOD popping artifacts
      // and would generally result in more detailed tree rendering than typically necessary
      // however, that means when one is facing away from a tree, but can see its shadow,
      // the shadow may disappear entirely, because no view is setting a decent LOD level
      //
      // by artificially increasing its bbox the main camera will affect the LOD much longer,
      // even when not looking at the tree, thus resulting in decent shadows

      bounds.m_fSphereRadius *= s_iLocalBoundsScale;
      bounds.m_vBoxHalfExtends *= (float)s_iLocalBoundsScale;
    }

    return EZ_SUCCESS;
  }

  return EZ_FAILURE;
}

void ezKrautTreeComponent::SetKrautTreeFile(const char* szFile)
{
  ezKrautTreeResourceHandle hTree;

  if (!ezStringUtils::IsNullOrEmpty(szFile))
  {
    hTree = ezResourceManager::LoadResource<ezKrautTreeResource>(szFile);
  }

  SetKrautTree(hTree);
}

const char* ezKrautTreeComponent::GetKrautTreeFile() const
{
  if (!m_hKrautTree.IsValid())
    return "";

  return m_hKrautTree.GetResourceID();
}

void ezKrautTreeComponent::SetKrautTree(const ezKrautTreeResourceHandle& hTree)
{
  if (m_hKrautTree != hTree)
  {
    m_hKrautTree = hTree;

    GetWorld()->GetOrCreateComponentManager<ezKrautTreeComponentManager>()->EnqueueUpdate(GetHandle());
  }
}

void ezKrautTreeComponent::Initialize()
{
  SUPER::Initialize();

  GetWorld()->GetOrCreateComponentManager<ezKrautTreeComponentManager>()->EnqueueUpdate(GetHandle());
}

void ezKrautTreeComponent::OnExtractRenderData(ezMsgExtractRenderData& msg) const
{
  if (!m_hKrautTree.IsValid())
    return;

  ezResourceLock<ezKrautTreeResource> pTree(m_hKrautTree, ezResourceAcquireMode::AllowFallback);

  // TODO: handle fallback case properly

  for (ezUInt8 uiCurLod = 0; uiCurLod < pTree->GetTreeLODs().GetCount(); ++uiCurLod)
  {
    const auto& lodData = pTree->GetTreeLODs()[uiCurLod];

    if (!lodData.m_hMesh.IsValid())
      continue;

    const ezUInt32 uiMeshIDHash = lodData.m_hMesh.GetResourceIDHash();

    ezResourceLock<ezMeshResource> pMesh(lodData.m_hMesh, ezResourceAcquireMode::AllowFallback);
    ezArrayPtr<const ezMeshResourceDescriptor::SubMesh> subMeshes = pMesh->GetSubMeshes();

    const ezGameObject* pOwner = GetOwner();

    float fGlobalUniformScale = pOwner->GetGlobalScalingSimd().HorizontalSum<3>() * ezSimdFloat(1.0f / 3.0f);

    const ezTransform tOwner = pOwner->GetGlobalTransform();
    const ezBoundingBoxSphere bounds = pOwner->GetGlobalBounds();
    const float fMinDistSQR = ezMath::Square(fGlobalUniformScale * lodData.m_fMinLodDistance);
    const float fMaxDistSQR = ezMath::Square(fGlobalUniformScale * lodData.m_fMaxLodDistance);

    for (ezUInt32 subMeshIdx = 0; subMeshIdx < subMeshes.GetCount(); ++subMeshIdx)
    {
      const auto& subMesh = subMeshes[subMeshIdx];

      const ezUInt32 uiMaterialIndex = subMesh.m_uiMaterialIndex;

      const ezMaterialResourceHandle& hMaterial = pMesh->GetMaterials()[uiMaterialIndex];
      const ezUInt32 uiMaterialIDHash = hMaterial.IsValid() ? hMaterial.GetResourceIDHash() : 0;

      // Generate batch id from mesh, material and part index.
      const ezUInt32 data[] = {uiMeshIDHash, uiMaterialIDHash, subMeshIdx, 0};
      const ezUInt32 uiBatchId = ezHashingUtils::xxHash32(data, sizeof(data));

      ezKrautRenderData* pRenderData = ezCreateRenderDataForThisFrame<ezKrautRenderData>(GetOwner());

      {
        pRenderData->m_uiBatchId = uiBatchId;
        pRenderData->m_uiSortingKey = (uiMaterialIDHash << 16) | (uiMeshIDHash & 0xFFFF);

        pRenderData->m_pTreeLodInfo = m_pLodInfo;
        pRenderData->m_uiThisLodIndex = uiCurLod;

        pRenderData->m_GlobalTransform = tOwner;
        pRenderData->m_GlobalBounds = bounds;
        pRenderData->m_hMesh = lodData.m_hMesh;
        pRenderData->m_uiSubMeshIndex = subMeshIdx;
        pRenderData->m_uiUniqueID = GetUniqueIdForRendering(uiMaterialIndex);
        pRenderData->m_bCastShadows = (lodData.m_LodType == ezKrautLodType::Mesh);

        pRenderData->m_vLeafCenter = pTree->GetDetails().m_vLeafCenter;
        pRenderData->m_fLodDistanceMinSQR = fMinDistSQR;
        pRenderData->m_fLodDistanceMaxSQR = fMaxDistSQR;
      }

      msg.AddRenderData(pRenderData, ezDefaultRenderDataCategories::LitOpaque, ezRenderData::Caching::IfStatic);
    }
  }
}

ezResult ezKrautTreeComponent::CreateGeometry(ezGeometry& geo, ezWorldGeoExtractionUtil::ExtractionMode mode) const
{
  if (GetOwner()->IsDynamic())
    return EZ_FAILURE;

  if (!m_hKrautTree.IsValid())
    return EZ_FAILURE;

  ezResourceLock<ezKrautTreeResource> pTree(m_hKrautTree, ezResourceAcquireMode::NoFallback);

  if (pTree.GetAcquireResult() != ezResourceAcquireResult::Final)
    return EZ_FAILURE;

  const auto& details = pTree->GetDetails();

  if (mode == ezWorldGeoExtractionUtil::ExtractionMode::RenderMesh)
  {
    // TODO: support to load the actual tree mesh and return it
  }

  const float fHeightScale = GetOwner()->GetGlobalScalingSimd().z();
  const float fMaxScale = GetOwner()->GetGlobalScalingSimd().HorizontalMax<3>();
  const float fRadius = details.m_fStaticColliderRadius * fMaxScale;

  if (fRadius <= 0.0f)
    return EZ_FAILURE;

  const float fTreeHeight = (details.m_Bounds.m_vCenter.z + details.m_Bounds.m_vBoxHalfExtends.z) * 0.9f;

  if (fHeightScale * fTreeHeight <= 0.0f)
    return EZ_FAILURE;

  // for the position offset we need to adjust for the tree scale (cylinder has its origin at its center)
  const ezMat4 transform = GetOwner()->GetGlobalTransform().GetAsMat4();

  // using a cone or even a cylinder with a thinner top results in the character controller getting stuck while sliding along the geometry
  // TODO: instead of triangle geometry it would maybe be better to use actual physics capsules

  // due to 'transform' this will already include the tree scale
  geo.AddCylinderOnePiece(fRadius, fRadius, fTreeHeight, 0.0f, 8, ezColor::White, transform);

  geo.TriangulatePolygons();

  return EZ_SUCCESS;
}

void ezKrautTreeComponent::OnExtractGeometry(ezMsgExtractGeometry& msg) const
{
  ezGeometry geo;
  if (CreateGeometry(geo, msg.m_Mode).Failed())
    return;

  auto& vertices = msg.m_pWorldGeometry->m_Vertices;
  auto& triangles = msg.m_pWorldGeometry->m_Triangles;

  const ezUInt32 uiFirstVertex = vertices.GetCount();

  for (const auto& vtx : geo.GetVertices())
  {
    vertices.ExpandAndGetRef().m_vPosition = vtx.m_vPosition;
  }

  for (const auto& tri : geo.GetPolygons())
  {
    auto& t = triangles.ExpandAndGetRef();
    t.m_uiVertexIndices[0] = uiFirstVertex + tri.m_Vertices[0];
    t.m_uiVertexIndices[1] = uiFirstVertex + tri.m_Vertices[1];
    t.m_uiVertexIndices[2] = uiFirstVertex + tri.m_Vertices[2];
  }
}

void ezKrautTreeComponent::OnBuildStaticMesh(ezMsgBuildStaticMesh& msg) const
{
  ezGeometry geo;
  if (CreateGeometry(geo, ezWorldGeoExtractionUtil::ExtractionMode::CollisionMesh).Failed())
    return;

  auto& desc = *msg.m_pStaticMeshDescription;
  auto& subMesh = msg.m_pStaticMeshDescription->m_SubMeshes.ExpandAndGetRef();

  {
    ezResourceLock<ezKrautTreeResource> pTree(m_hKrautTree, ezResourceAcquireMode::NoFallback);
    const auto& details = pTree->GetDetails();

    if (!details.m_sSurfaceResource.IsEmpty())
    {
      subMesh.m_uiSurfaceIndex = desc.m_Surfaces.GetCount();
      desc.m_Surfaces.PushBack(details.m_sSurfaceResource);
    }
  }

  subMesh.m_uiFirstTriangle = desc.m_Triangles.GetCount();
  subMesh.m_uiNumTriangles = geo.GetPolygons().GetCount();

  const ezUInt32 uiFirstVertex = desc.m_Vertices.GetCount();

  for (const auto& vtx : geo.GetVertices())
  {
    desc.m_Vertices.ExpandAndGetRef() = vtx.m_vPosition;
  }

  for (const auto& tri : geo.GetPolygons())
  {
    auto& t = desc.m_Triangles.ExpandAndGetRef();
    t.m_uiVertexIndices[0] = uiFirstVertex + tri.m_Vertices[0];
    t.m_uiVertexIndices[1] = uiFirstVertex + tri.m_Vertices[1];
    t.m_uiVertexIndices[2] = uiFirstVertex + tri.m_Vertices[2];
  }
}

//////////////////////////////////////////////////////////////////////////

void ezKrautTreeComponentManager::Initialize()
{
  SUPER::Initialize();

  ezWorldModule::UpdateFunctionDesc desc = EZ_CREATE_MODULE_UPDATE_FUNCTION_DESC(ezKrautTreeComponentManager::Update, this);
  desc.m_Phase = UpdateFunctionDesc::Phase::PreAsync;

  RegisterUpdateFunction(desc);

  ezResourceManager::s_ResourceEvents.AddEventHandler(ezMakeDelegate(&ezKrautTreeComponentManager::ResourceEventHandler, this));
}

void ezKrautTreeComponentManager::Deinitialize()
{
  EZ_LOCK(m_Mutex);

  ezResourceManager::s_ResourceEvents.RemoveEventHandler(ezMakeDelegate(&ezKrautTreeComponentManager::ResourceEventHandler, this));

  SUPER::Deinitialize();
}

void ezKrautTreeComponentManager::Update(const ezWorldModule::UpdateContext& context)
{
  for (const auto& hComp : m_RequireUpdate)
  {
    ezKrautTreeComponent* pComp = nullptr;
    if (!TryGetComponent(hComp, pComp))
      continue;

    pComp->TriggerLocalBoundsUpdate();
  }

  m_RequireUpdate.Clear();
}

void ezKrautTreeComponentManager::EnqueueUpdate(ezComponentHandle hComponent)
{
  m_RequireUpdate.PushBack(hComponent);
}

void ezKrautTreeComponentManager::ResourceEventHandler(const ezResourceEvent& e)
{
  if ((e.m_Type == ezResourceEvent::Type::ResourceContentUnloading || e.m_Type == ezResourceEvent::Type::ResourceContentUpdated) &&
      e.m_pResource->GetDynamicRTTI()->IsDerivedFrom<ezKrautTreeResource>())
  {
    EZ_LOCK(m_Mutex);

    ezKrautTreeResourceHandle hResource((ezKrautTreeResource*)(e.m_pResource));

    for (auto it = m_Components.GetIterator(); it.IsValid(); ++it)
    {
      const ezKrautTreeComponent* pComponent = static_cast<ezKrautTreeComponent*>(it.Value());

      if (pComponent->GetKrautTree() == hResource)
      {
        m_RequireUpdate.PushBack(pComponent->GetHandle());
      }
    }
  }
}
