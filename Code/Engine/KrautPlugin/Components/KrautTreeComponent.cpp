#include <PCH.h>

#include <Core/Graphics/Geometry.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Core/Utils/WorldGeoExtractionUtil.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
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
      bounds.m_vBoxHalfExtends *= s_iLocalBoundsScale;
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

ezKrautRenderData* ezKrautTreeComponent::CreateBranchRenderData(ezUInt32 uiBatchId) const
{
  return ezCreateRenderDataForThisFrame<ezKrautRenderData>(GetOwner(), uiBatchId);
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

    ezResourceLock<ezMeshResource> pMesh(lodData.m_hMesh);
    ezArrayPtr<const ezMeshResourceDescriptor::SubMesh> subMeshes = pMesh->GetSubMeshes();

    for (ezUInt32 subMeshIdx = 0; subMeshIdx < subMeshes.GetCount(); ++subMeshIdx)
    {
      const auto& subMesh = subMeshes[subMeshIdx];

      const ezUInt32 uiMaterialIndex = subMesh.m_uiMaterialIndex;

      const ezMaterialResourceHandle& hMaterial = pMesh->GetMaterials()[uiMaterialIndex];
      const ezUInt32 uiMaterialIDHash = hMaterial.IsValid() ? hMaterial.GetResourceIDHash() : 0;

      // Generate batch id from mesh, material and part index.
      ezUInt32 data[] = {uiMeshIDHash, uiMaterialIDHash, subMeshIdx, 0};
      ezUInt32 uiBatchId = ezHashing::xxHash32(data, sizeof(data));

      ezKrautRenderData* pRenderData = CreateBranchRenderData(uiBatchId);

      {
        pRenderData->m_pTreeLodInfo = m_pLodInfo;
        pRenderData->m_uiThisLodIndex = uiCurLod;

        pRenderData->m_GlobalTransform = GetOwner()->GetGlobalTransform();
        pRenderData->m_GlobalBounds = GetOwner()->GetGlobalBounds();
        pRenderData->m_hMesh = lodData.m_hMesh;
        pRenderData->m_uiSubMeshIndex = subMeshIdx;
        pRenderData->m_uiUniqueID = GetUniqueIdForRendering(uiMaterialIndex);
        pRenderData->m_bCastShadows = (lodData.m_LodType == ezKrautLodType::Mesh);

        pRenderData->m_vLeafCenter = pTree->GetDetails().m_vLeafCenter;
        pRenderData->m_fLodDistanceMinSQR = ezMath::Square(lodData.m_fMinLodDistance);
        pRenderData->m_fLodDistanceMaxSQR = ezMath::Square(lodData.m_fMaxLodDistance);
      }

      // Sort by material and then by mesh
      ezUInt32 uiSortingKey = (uiMaterialIDHash << 16) | (uiMeshIDHash & 0xFFFE);
      msg.AddRenderData(pRenderData, ezDefaultRenderDataCategories::LitOpaque, uiSortingKey, ezRenderData::Caching::IfStatic);
    }
  }
}

void ezKrautTreeComponent::OnExtractGeometry(ezMsgExtractGeometry& msg) const
{
  if (GetOwner()->IsDynamic())
    return;

  if (!m_hKrautTree.IsValid())
    return;

  ezResourceLock<ezKrautTreeResource> pTree(m_hKrautTree, ezResourceAcquireMode::NoFallback);

  if (pTree.GetAcquireResult() != ezResourceAcquireResult::Final)
    return;

  if (pTree->GetDetails().m_fNavMeshFootprint <= 0.0f)
    return;

  const ezVec3 vScale = ezSimdConversion::ToVec3(GetOwner()->GetGlobalTransformSimd().m_Scale.Abs());

  ezMat4 transform = GetOwner()->GetGlobalTransform().GetAsMat4();
  transform.SetTranslationVector(transform.GetTranslationVector() + ezVec3(0, 0, 1));

  ezGeometry geo;
  geo.AddCylinder(pTree->GetDetails().m_fNavMeshFootprint, pTree->GetDetails().m_fNavMeshFootprint, 2.0f, false, false, 8, ezColor::White,
                  transform);

  auto& vertices = msg.m_pWorldGeometry->m_Vertices;
  auto& triangles = msg.m_pWorldGeometry->m_Triangles;

  const ezUInt32 uiFirstVertex = vertices.GetCount();

  geo.TriangulatePolygons();

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
  if ((e.m_EventType == ezResourceEventType::ResourceContentUnloading || e.m_EventType == ezResourceEventType::ResourceContentUpdated) &&
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
