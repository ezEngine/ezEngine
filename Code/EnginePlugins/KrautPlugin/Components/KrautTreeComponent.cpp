#include <KrautPlugin/KrautPluginPCH.h>

#include <Core/Graphics/Geometry.h>
#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/Interfaces/WindWorldModule.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <KrautPlugin/Components/KrautTreeComponent.h>
#include <KrautPlugin/Renderer/KrautRenderData.h>
#include <KrautPlugin/Resources/KrautGeneratorResource.h>
#include <KrautPlugin/Resources/KrautTreeResource.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Meshes/CpuMeshResource.h>
#include <RendererCore/Meshes/MeshComponentBase.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererCore/Utils/WorldGeoExtractionUtil.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezKrautTreeComponent, 3, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_RESOURCE_ACCESSOR_PROPERTY("KrautTree", GetKrautGeneratorResource, SetKrautGeneratorResource)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Kraut_Tree")),
    EZ_ACCESSOR_PROPERTY("VariationIndex", GetVariationIndex, SetVariationIndex)->AddAttributes(new ezDefaultValueAttribute(0xFFFF)),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgExtractRenderData, OnMsgExtractRenderData),
    EZ_MESSAGE_HANDLER(ezMsgExtractGeometry, OnMsgExtractGeometry),
    EZ_MESSAGE_HANDLER(ezMsgBuildStaticMesh, OnBuildStaticMesh),
  }
  EZ_END_MESSAGEHANDLERS;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Terrain"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezKrautTreeComponent::ezKrautTreeComponent() = default;
ezKrautTreeComponent::~ezKrautTreeComponent() = default;

void ezKrautTreeComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();

  s << m_hKrautGenerator;
  s << m_uiVariationIndex;
  s << m_uiCustomRandomSeed;
}

void ezKrautTreeComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const ezUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = inout_stream.GetStream();

  if (uiVersion <= 1)
  {
    s >> m_hKrautTree;
  }
  else
  {
    s >> m_hKrautGenerator;
  }

  s >> m_uiVariationIndex;
  s >> m_uiCustomRandomSeed;

  if (uiVersion == 2)
  {
    ezUInt16 m_uiDefaultVariationIndex;
    s >> m_uiDefaultVariationIndex;
  }

  GetWorld()->GetOrCreateComponentManager<ezKrautTreeComponentManager>()->EnqueueUpdate(GetHandle());
}

ezResult ezKrautTreeComponent::GetLocalBounds(ezBoundingBoxSphere& bounds, bool& bAlwaysVisible, ezMsgUpdateLocalBounds& msg)
{
  if (m_hKrautTree.IsValid())
  {
    ezResourceLock<ezKrautTreeResource> pTree(m_hKrautTree, ezResourceAcquireMode::AllowLoadingFallback);
    // TODO: handle fallback case properly

    bounds = pTree->GetDetails().m_Bounds;

    {
      // this is a work around to make shadows and LODing work better
      // shadows do not affect the maximum LOD of a tree that is being rendered,
      // otherwise moving/rotating light-sources would cause LOD popping artifacts
      // and would generally result in more detailed tree rendering than typically necessary
      // however, that means when one is facing away from a tree, but can see its shadow,
      // the shadow may disappear entirely, because no view is setting a decent LOD level
      //
      // by artificially increasing its bbox the main camera will affect the LOD much longer,
      // even when not looking at the tree, thus resulting in decent shadows

      bounds.m_fSphereRadius *= s_iLocalBoundsScale;
      bounds.m_vBoxHalfExtents *= (float)s_iLocalBoundsScale;
    }

    return EZ_SUCCESS;
  }

  return EZ_FAILURE;
}

void ezKrautTreeComponent::SetVariationIndex(ezUInt16 uiIndex)
{
  if (m_uiVariationIndex == uiIndex)
    return;

  m_uiVariationIndex = uiIndex;

  if (IsActiveAndInitialized() && m_hKrautGenerator.IsValid())
  {
    GetWorld()->GetOrCreateComponentManager<ezKrautTreeComponentManager>()->EnqueueUpdate(GetHandle());
  }
}

ezUInt16 ezKrautTreeComponent::GetVariationIndex() const
{
  return m_uiVariationIndex;
}

void ezKrautTreeComponent::SetCustomRandomSeed(ezUInt16 uiSeed)
{
  if (m_uiCustomRandomSeed == uiSeed)
    return;

  m_uiCustomRandomSeed = uiSeed;

  if (IsActiveAndInitialized() && m_hKrautGenerator.IsValid())
  {
    GetWorld()->GetOrCreateComponentManager<ezKrautTreeComponentManager>()->EnqueueUpdate(GetHandle());
  }
}

ezUInt16 ezKrautTreeComponent::GetCustomRandomSeed() const
{
  return m_uiCustomRandomSeed;
}

void ezKrautTreeComponent::SetKrautGeneratorResource(const ezKrautGeneratorResourceHandle& hTree)
{
  if (m_hKrautGenerator == hTree)
    return;

  m_hKrautGenerator = hTree;

  if (IsActiveAndInitialized())
  {
    GetWorld()->GetOrCreateComponentManager<ezKrautTreeComponentManager>()->EnqueueUpdate(GetHandle());
  }
}

void ezKrautTreeComponent::OnActivated()
{
  SUPER::OnActivated();

  m_hKrautTree.Invalidate();
  m_vWindSpringPos.SetZero();
  m_vWindSpringVel.SetZero();

  if (m_hKrautGenerator.IsValid())
  {
    GetWorld()->GetOrCreateComponentManager<ezKrautTreeComponentManager>()->EnqueueUpdate(GetHandle());
  }
}

void ezKrautTreeComponent::OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const
{
  if (!m_hKrautTree.IsValid())
    return;

  ezResourceLock<ezKrautTreeResource> pTree(m_hKrautTree, ezResourceAcquireMode::AllowLoadingFallback);

  // if (pTree.GetAcquireResult() != ezResourceAcquireResult::Final)
  //   return;

  ComputeWind();

  // ignore scale, the shader expects the wind strength in the global 0-20 m/sec range
  const ezVec3 vLocalWind = GetOwner()->GetGlobalRotation().GetInverse() * m_vWindSpringPos;

  const ezUInt8 uiMaxLods = static_cast<ezUInt8>(pTree->GetTreeLODs().GetCount());
  for (ezUInt8 uiCurLod = 0; uiCurLod < uiMaxLods; ++uiCurLod)
  {
    const auto& lodData = pTree->GetTreeLODs()[uiCurLod];

    if (!lodData.m_hMesh.IsValid())
      continue;

    const ezUInt32 uiMeshIDHash = ezHashingUtils::StringHashTo32(lodData.m_hMesh.GetResourceIDHash());

    ezResourceLock<ezMeshResource> pMesh(lodData.m_hMesh, ezResourceAcquireMode::AllowLoadingFallback);
    ezArrayPtr<const ezMeshResourceDescriptor::SubMesh> subMeshes = pMesh->GetSubMeshes();

    const ezGameObject* pOwner = GetOwner();

    float fGlobalUniformScale = pOwner->GetGlobalScalingSimd().HorizontalSum<3>() * ezSimdFloat(1.0f / 3.0f);

    const ezTransform tOwner = pOwner->GetGlobalTransform();
    const ezBoundingBoxSphere bounds = pOwner->GetGlobalBounds();
    const float fMinDistSQR = ezMath::Square(fGlobalUniformScale * lodData.m_fMinLodDistance);
    const float fMaxDistSQR = ezMath::Square(fGlobalUniformScale * lodData.m_fMaxLodDistance);

    for (ezUInt32 subMeshIdx = 0; subMeshIdx < subMeshes.GetCount(); ++subMeshIdx)
    {
      ezKrautRenderData* pRenderData = ezCreateRenderDataForThisFrame<ezKrautRenderData>(GetOwner());

      {
        pRenderData->m_uiSortingKey = uiMeshIDHash + subMeshIdx;

        pRenderData->m_uiThisLodIndex = uiCurLod;

        pRenderData->m_GlobalTransform = tOwner;
        pRenderData->m_GlobalBounds = bounds;
        pRenderData->m_hMesh = lodData.m_hMesh;
        pRenderData->m_uiSubMeshIndex = static_cast<ezUInt8>(subMeshIdx);
        pRenderData->m_uiUniqueID = GetUniqueIdForRendering(0);
        pRenderData->m_bCastShadows = (lodData.m_LodType == ezKrautLodType::Mesh);

        pRenderData->m_vLeafCenter = pTree->GetDetails().m_vLeafCenter;
        pRenderData->m_fLodDistanceMinSQR = fMinDistSQR;
        pRenderData->m_fLodDistanceMaxSQR = fMaxDistSQR;

        pRenderData->m_vWindTrunk = vLocalWind;
        pRenderData->m_vWindBranches = vLocalWind;
      }

      // TODO: somehow make Kraut render data static again and pass along the wind vectors differently
      msg.AddRenderData(pRenderData, ezDefaultRenderDataCategories::LitOpaque, ezRenderData::Caching::Never);
    }
  }
}

ezResult ezKrautTreeComponent::CreateGeometry(ezGeometry& geo, ezWorldGeoExtractionUtil::ExtractionMode mode) const
{
  if (GetOwner()->IsDynamic())
    return EZ_FAILURE;

  // EnsureTreeIsGenerated(); // not const

  if (!m_hKrautTree.IsValid())
    return EZ_FAILURE;

  ezResourceLock<ezKrautTreeResource> pTree(m_hKrautTree, ezResourceAcquireMode::BlockTillLoaded_NeverFail);

  if (pTree.GetAcquireResult() != ezResourceAcquireResult::Final)
    return EZ_FAILURE;

  const auto& details = pTree->GetDetails();

  if (mode == ezWorldGeoExtractionUtil::ExtractionMode::RenderMesh)
  {
    // TODO: support to load the actual tree mesh and return it
  }
  // else
  {
    const float fHeightScale = GetOwner()->GetGlobalScalingSimd().z();
    const float fMaxScale = GetOwner()->GetGlobalScalingSimd().HorizontalMax<3>();

    if (details.m_fStaticColliderRadius * fMaxScale <= 0.0f)
      return EZ_FAILURE;

    const float fTreeHeight = (details.m_Bounds.m_vCenter.z + details.m_Bounds.m_vBoxHalfExtents.z) * 0.9f;

    if (fHeightScale * fTreeHeight <= 0.0f)
      return EZ_FAILURE;

    // using a cone or even a cylinder with a thinner top results in the character controller getting stuck while sliding along the geometry
    // TODO: instead of triangle geometry it would maybe be better to use actual physics capsules

    // due to 'transform' this will already include the tree scale
    geo.AddCylinderOnePiece(details.m_fStaticColliderRadius, details.m_fStaticColliderRadius, fTreeHeight, 0.0f, 8);

    geo.TriangulatePolygons();
  }

  return EZ_SUCCESS;
}

void ezKrautTreeComponent::EnsureTreeIsGenerated()
{
  if (!m_hKrautGenerator.IsValid())
    return;

  ezResourceLock<ezKrautGeneratorResource> pResource(m_hKrautGenerator, ezResourceAcquireMode::BlockTillLoaded_NeverFail);

  if (pResource.GetAcquireResult() != ezResourceAcquireResult::Final)
    return;

  ezKrautTreeResourceHandle hNewTree;

  if (m_uiCustomRandomSeed != 0xFFFF)
  {
    hNewTree = pResource->GenerateTree(m_uiCustomRandomSeed);
  }
  else
  {
    if (m_uiVariationIndex == 0xFFFF)
    {
      hNewTree = pResource->GenerateTreeWithGoodSeed(GetOwner()->GetStableRandomSeed() & 0xFFFF);
    }
    else
    {
      hNewTree = pResource->GenerateTreeWithGoodSeed(m_uiVariationIndex);
    }
  }

  if (m_hKrautTree != hNewTree)
  {
    m_hKrautTree = hNewTree;
    TriggerLocalBoundsUpdate();
  }
}

void ezKrautTreeComponent::ComputeWind() const
{
  if (!IsActiveAndSimulating())
    return;

  // ComputeWind() is called by the renderer extraction, which happens once for every view
  // make sure the wind update happens only once per frame, otherwise the spring would behave differently
  // depending on how many light sources (with shadows) shine on a tree
  if (ezRenderWorld::GetFrameCounter() == m_uiLastWindUpdate)
    return;

  m_uiLastWindUpdate = ezRenderWorld::GetFrameCounter();

  const ezWindWorldModuleInterface* pWindInterface = GetWorld()->GetModuleReadOnly<ezWindWorldModuleInterface>();

  if (!pWindInterface)
    return;

  auto pOwnder = GetOwner();

  const ezVec3 vOwnerPos = pOwnder->GetGlobalPosition();
  const ezVec3 vSampleWindPos = vOwnerPos + ezVec3(0, 0, 2);
  const ezVec3 vWindForce = pWindInterface->GetWindAt(vSampleWindPos);

  const float realTimeStep = GetWorld()->GetClock().GetTimeDiff().AsFloatInSeconds();

  // springy wind force
  {
    const float fOverallStrength = 4.0f;

    const float fSpringConstant = 1.0f;
    const float fSpringDamping = 0.5f;
    const float fTreeMass = 1.0f;

    const ezVec3 vSpringForce = -(fSpringConstant * m_vWindSpringPos + fSpringDamping * m_vWindSpringVel);

    const ezVec3 vTotalForce = vWindForce + vSpringForce;

    // F = mass*acc
    // acc = F / mass
    const ezVec3 vTreeAcceleration = vTotalForce / fTreeMass;

    m_vWindSpringVel += vTreeAcceleration * realTimeStep * fOverallStrength;
    m_vWindSpringPos += m_vWindSpringVel * realTimeStep * fOverallStrength;
  }

  // debug draw wind vectors
  if (false)
  {
    const ezVec3 offset = GetOwner()->GetGlobalPosition() + ezVec3(2, 0, 1);

    ezHybridArray<ezDebugRendererLine, 2> lines;

    // actual wind
    {
      auto& l = lines.ExpandAndGetRef();
      l.m_start = offset;
      l.m_end = offset + vWindForce;
      l.m_startColor = ezColor::BlueViolet;
      l.m_endColor = ezColor::PowderBlue;
    }

    // springy wind
    {
      auto& l = lines.ExpandAndGetRef();
      l.m_start = offset;
      l.m_end = offset + m_vWindSpringPos;
      l.m_startColor = ezColor::BlueViolet;
      l.m_endColor = ezColor::MediumVioletRed;
    }

    // springy wind 2
    {
      auto& l = lines.ExpandAndGetRef();
      l.m_start = offset;
      l.m_end = offset + m_vWindSpringPos;
      l.m_startColor = ezColor::LightGoldenRodYellow;
      l.m_endColor = ezColor::MediumVioletRed;
    }

    ezDebugRenderer::DrawLines(GetWorld(), lines, ezColor::White);

    ezStringBuilder tmp;
    tmp.SetFormat("Wind: {}m/s", m_vWindSpringPos.GetLength());

    ezDebugRenderer::Draw3DText(GetWorld(), tmp, GetOwner()->GetGlobalPosition() + ezVec3(0, 0, 1), ezColor::DeepSkyBlue);
  }
}

void ezKrautTreeComponent::OnMsgExtractGeometry(ezMsgExtractGeometry& ref_msg) const
{
  ezStringBuilder sResourceName;
  sResourceName.SetFormat("KrautTreeCpu:{}", m_hKrautGenerator.GetResourceID());

  ezCpuMeshResourceHandle hMesh = ezResourceManager::GetExistingResource<ezCpuMeshResource>(sResourceName);
  if (!hMesh.IsValid())
  {
    ezGeometry geo;
    if (CreateGeometry(geo, ref_msg.m_Mode).Failed())
      return;

    ezMeshResourceDescriptor desc;

    desc.MeshBufferDesc().AddCommonStreams();
    desc.MeshBufferDesc().AllocateStreamsFromGeometry(geo, ezGALPrimitiveTopology::Triangles);

    desc.AddSubMesh(desc.MeshBufferDesc().GetPrimitiveCount(), 0, 0);

    desc.ComputeBounds();

    hMesh = ezResourceManager::GetOrCreateResource<ezCpuMeshResource>(sResourceName, std::move(desc), sResourceName);
  }

  ref_msg.AddMeshObject(GetOwner()->GetGlobalTransform(), hMesh);
}

void ezKrautTreeComponent::OnBuildStaticMesh(ezMsgBuildStaticMesh& ref_msg) const
{
  ezGeometry geo;
  if (CreateGeometry(geo, ezWorldGeoExtractionUtil::ExtractionMode::CollisionMesh).Failed())
    return;

  auto& desc = *ref_msg.m_pStaticMeshDescription;
  auto& subMesh = ref_msg.m_pStaticMeshDescription->m_SubMeshes.ExpandAndGetRef();

  {
    ezResourceLock<ezKrautTreeResource> pTree(m_hKrautTree, ezResourceAcquireMode::BlockTillLoaded_NeverFail);

    if (pTree.GetAcquireResult() != ezResourceAcquireResult::Final)
      return;

    const auto& details = pTree->GetDetails();

    if (!details.m_sSurfaceResource.IsEmpty())
    {
      subMesh.m_uiSurfaceIndex = static_cast<ezUInt16>(desc.m_Surfaces.GetCount());
      desc.m_Surfaces.PushBack(details.m_sSurfaceResource);
    }
  }

  const ezTransform transform = GetOwner()->GetGlobalTransform();

  subMesh.m_uiFirstTriangle = desc.m_Triangles.GetCount();
  subMesh.m_uiNumTriangles = geo.GetPolygons().GetCount();

  const ezUInt32 uiFirstVertex = desc.m_Vertices.GetCount();

  for (const auto& vtx : geo.GetVertices())
  {
    desc.m_Vertices.ExpandAndGetRef() = transform.TransformPosition(vtx.m_vPosition);
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
  desc.m_Phase = ezWorldUpdatePhase::PreAsync;

  RegisterUpdateFunction(desc);

  ezResourceManager::GetResourceEvents().AddEventHandler(ezMakeDelegate(&ezKrautTreeComponentManager::ResourceEventHandler, this));
}

void ezKrautTreeComponentManager::Deinitialize()
{
  EZ_LOCK(m_Mutex);

  ezResourceManager::GetResourceEvents().RemoveEventHandler(ezMakeDelegate(&ezKrautTreeComponentManager::ResourceEventHandler, this));

  SUPER::Deinitialize();
}

void ezKrautTreeComponentManager::Update(const ezWorldModule::UpdateContext& context)
{
  ezDeque<ezComponentHandle> requireUpdate;

  {
    EZ_LOCK(m_Mutex);
    requireUpdate.Swap(m_RequireUpdate);
  }

  for (const auto& hComp : requireUpdate)
  {
    ezKrautTreeComponent* pComp = nullptr;
    if (!TryGetComponent(hComp, pComp) || !pComp->IsActiveAndInitialized())
      continue;

    // TODO: this could be wrapped into a task
    pComp->EnsureTreeIsGenerated();
  }
}

void ezKrautTreeComponentManager::EnqueueUpdate(ezComponentHandle hComponent)
{
  EZ_LOCK(m_Mutex);

  if (m_RequireUpdate.IndexOf(hComponent) != ezInvalidIndex)
    return;

  m_RequireUpdate.PushBack(hComponent);
}

void ezKrautTreeComponentManager::ResourceEventHandler(const ezResourceEvent& e)
{
  if ((e.m_Type == ezResourceEvent::Type::ResourceContentUnloading || e.m_Type == ezResourceEvent::Type::ResourceContentUpdated) && e.m_pResource->GetDynamicRTTI()->IsDerivedFrom<ezKrautGeneratorResource>())
  {
    EZ_LOCK(m_Mutex);

    ezKrautGeneratorResourceHandle hResource((ezKrautGeneratorResource*)(e.m_pResource));

    for (auto it = m_Components.GetIterator(); it.IsValid(); ++it)
    {
      const ezKrautTreeComponent* pComponent = static_cast<ezKrautTreeComponent*>(it.Value());

      if (pComponent->GetKrautGeneratorResource() == hResource)
      {
        EnqueueUpdate(pComponent->GetHandle());
      }
    }
  }
}


EZ_STATICLINK_FILE(KrautPlugin, KrautPlugin_Components_KrautTreeComponent);
