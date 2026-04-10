#include <KrautPlugin/KrautPluginPCH.h>

#include <Core/Graphics/Geometry.h>
#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/Interfaces/WindWorldModule.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <KrautPlugin/Components/KrautTreeComponent.h>
#include <KrautPlugin/Resources/KrautGeneratorResource.h>
#include <KrautPlugin/Resources/KrautTreeResource.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Meshes/CpuMeshResource.h>
#include <RendererCore/Meshes/MeshComponentBase.h>
#include <RendererCore/Pipeline/RenderDataManager.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/Utils/WorldGeoExtractionUtil.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_BITFLAGS(ezKrautTreeTypeBits, 1)
  EZ_BITFLAGS_CONSTANTS(ezKrautTreeTypeBits::Trunk1, ezKrautTreeTypeBits::Trunk2, ezKrautTreeTypeBits::Trunk3)
  EZ_BITFLAGS_CONSTANTS(ezKrautTreeTypeBits::MainBranches1, ezKrautTreeTypeBits::MainBranches2, ezKrautTreeTypeBits::MainBranches3)
  EZ_BITFLAGS_CONSTANTS(ezKrautTreeTypeBits::SubBranches1, ezKrautTreeTypeBits::SubBranches2, ezKrautTreeTypeBits::SubBranches3)
  EZ_BITFLAGS_CONSTANTS(ezKrautTreeTypeBits::Twigs1, ezKrautTreeTypeBits::Twigs2, ezKrautTreeTypeBits::Twigs3)
EZ_END_STATIC_REFLECTED_BITFLAGS

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
  if (!m_hKrautTree.IsValid())
    return EZ_FAILURE;

  ezResourceLock<ezKrautTreeResource> pTree(m_hKrautTree, ezResourceAcquireMode::PointerOnly);
  if (!pTree.IsValid())
    return EZ_FAILURE;

  const ezBoundingBoxSphere treeBounds = pTree->GetDetails().m_Bounds;
  if (!treeBounds.IsValid())
    return EZ_FAILURE; // base data not yet generated; re-trigger once available

  bounds = treeBounds;

  // Artificially inflate the bounds so the main camera keeps selecting a decent LOD even when
  // the tree is not directly in view (e.g. for correct shadow LOD selection).
  bounds.m_fSphereRadius *= s_iLocalBoundsScale;
  bounds.m_vBoxHalfExtents *= (float)s_iLocalBoundsScale;

  return EZ_SUCCESS;
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
  if (!m_hKrautTree.IsValid() || !m_hKrautGenerator.IsValid())
    return;

  ezResourceLock<ezKrautTreeResource> pTree(m_hKrautTree, ezResourceAcquireMode::PointerOnly);
  if (!pTree.IsValid())
    return;

  const ezGameObject* pOwner = GetOwner();
  const ezTransform tOwner = pOwner->GetGlobalTransform();
  const float fGlobalUniformScale = pOwner->GetGlobalScalingSimd().HorizontalSum<3>() * ezSimdFloat(1.0f / 3.0f);

  const ezVec3 vLodCamPos = msg.m_pView->GetLodCamera()->GetPosition();
  const bool bIsShadowView = msg.m_pView->GetCameraUsageHint() == ezCameraUsageHint::Shadow;
  const float fDistanceSQR = (tOwner.m_vPosition - vLodCamPos).GetLengthSquared();

  const ezUInt8 uiMaxLods = static_cast<ezUInt8>(pTree->GetTreeLODs().GetCount());

  // Determine which single LOD to render
  ezUInt8 uiActiveLod = uiMaxLods; // sentinel: no LOD selected yet

  if (m_iLodOverride >= 0 && m_iLodOverride < uiMaxLods)
  {
    uiActiveLod = static_cast<ezUInt8>(m_iLodOverride);
  }
  else
  {
    // Skip LOD0 (full-detail) in distance-based selection; it is only shown via override.
    // LODs 1..N are the runtime LODs.
    for (ezUInt8 uiCurLod = 1; uiCurLod < uiMaxLods; ++uiCurLod)
    {
      const auto& lodData = pTree->GetTreeLODs()[uiCurLod];
      const float fMinDistSQR = ezMath::Square(fGlobalUniformScale * lodData.m_fMinLodDistance);
      const float fMaxDistSQR = ezMath::Square(fGlobalUniformScale * lodData.m_fMaxLodDistance);

      if (fDistanceSQR >= fMinDistSQR && fDistanceSQR < fMaxDistSQR)
      {
        uiActiveLod = uiCurLod;
        break;
      }
    }

    if (uiActiveLod == uiMaxLods)
      return; // beyond all LOD distances: don't render
  }

  // Request the LOD mesh — returns true if ready, false if still generating.
  {
    ezResourceLock<ezKrautGeneratorResource> pGenerator(m_hKrautGenerator, ezResourceAcquireMode::PointerOnly);
    if (!pGenerator.IsValid())
      return;

    if (!pGenerator->RequestLodMesh(m_hKrautTree, m_uiCurrentSeed, uiActiveLod, m_bForceGenerateImmediate))
    {
      // In runtime, fall back to a coarser LOD (higher index) that may already be ready,
      // rather than skipping rendering entirely for this frame.
      bool bFoundFallback = false;
      for (ezUInt8 uiFallbackLod = uiActiveLod + 1; uiFallbackLod < uiMaxLods; ++uiFallbackLod)
      {
        if (pGenerator->RequestLodMesh(m_hKrautTree, m_uiCurrentSeed, uiFallbackLod, false))
        {
          uiActiveLod = uiFallbackLod;
          bFoundFallback = true;
          break;
        }
      }

      if (!bFoundFallback)
        return; // no LOD ready yet; skip this frame
    }
  }

  m_iLastRenderedLod = static_cast<ezInt8>(uiActiveLod);

  const auto& lodData = pTree->GetTreeLODs()[uiActiveLod];
  if (!lodData.m_hMesh.IsValid())
    return;

  if (bIsShadowView && lodData.m_LodType != ezKrautLodType::Mesh)
    return;

  // ignore scale, the shader expects the wind strength in the global 0-20 m/sec range
  const ezVec3 vLocalWind = pOwner->GetGlobalRotation().GetInverse() * m_vWindSpringPos;

  const bool bDynamic = true;
  const ezColor color = ezColor(vLocalWind.x, vLocalWind.y, vLocalWind.z, vLocalWind.GetLength());
  const ezVec4 customData = pTree->GetDetails().m_vLeafCenter.GetAsVec4(0.0f);
  auto hInstanceDataBuffer = msg.m_pRenderDataManager->GetOrCreateInstanceDataAndFill(*this, bDynamic, tOwner, m_InstanceDataOffset, GetUniqueIdForRendering(), color, customData);

  ezResourceLock<ezMeshResource> pMesh(lodData.m_hMesh, ezResourceAcquireMode::AllowLoadingFallback);
  ezArrayPtr<const ezMeshResourceDescriptor::SubMesh> subMeshes = pMesh->GetSubMeshes();
  ezArrayPtr<const ezKrautTreeResourceDescriptor::MaterialData> materials = lodData.m_Materials;

  for (ezUInt32 subMeshIdx = 0; subMeshIdx < subMeshes.GetCount(); ++subMeshIdx)
  {
    const ezUInt32 uiMaterialIndex = subMeshes[subMeshIdx].m_uiMaterialIndex;

    if (uiMaterialIndex < materials.GetCount())
    {
      const auto& matInfo = materials[uiMaterialIndex];

      if (matInfo.m_BranchType != ezKrautBranchType::None && m_bHideFrondsAndLeafs &&
          (matInfo.m_MaterialType == ezKrautMaterialType::Frond || matInfo.m_MaterialType == ezKrautMaterialType::Leaf))
      {
        continue;
      }
    }

    ezMaterialResourceHandle hMaterial = pMesh->GetMaterials()[uiMaterialIndex];

    ezMeshRenderData* pRenderData = msg.m_pRenderDataManager->CreateRenderDataForThisFrame<ezMeshRenderData>(pOwner);
    pRenderData->SetFallbackGlobalBounds(GetOwner()->GetGlobalBounds());
    pRenderData->Fill(m_InstanceDataOffset, hInstanceDataBuffer, hMaterial, lodData.m_hMesh, uiMaterialIndex, subMeshIdx);

    msg.AddRenderData(pRenderData, ezDefaultRenderDataCategories::LitOpaque, ezRenderData::Caching::Never);
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

  auto pDesc = pResource->GetDescriptor();
  if (pDesc == nullptr)
    return;

  // Compute the seed for this component instance
  ezUInt32 uiSeed;
  if (m_uiCustomRandomSeed != 0xFFFF)
  {
    uiSeed = m_uiCustomRandomSeed;
  }
  else
  {
    const ezUInt16 uiIndex = (m_uiVariationIndex != 0xFFFF) ? m_uiVariationIndex : static_cast<ezUInt16>(GetOwner()->GetStableRandomSeed() & 0xFFFF);
    if (pDesc->m_GoodRandomSeeds.IsEmpty())
      uiSeed = uiIndex;
    else
      uiSeed = pDesc->m_GoodRandomSeeds[uiIndex % pDesc->m_GoodRandomSeeds.GetCount()];
  }

  const ezKrautTreeResourceHandle hNewTree = pResource->GetOrCreateTreeResource(uiSeed);

  if (m_hKrautTree == hNewTree)
  {
    m_uiCurrentSeed = uiSeed;
    return;
  }

  // A new tree resource is needed (generator content changed or first-time setup).
  // If we were already rendering a specific LOD, wait until that LOD is ready in the new
  // tree before switching, so the old tree keeps rendering without flickering.
  if (m_iLastRenderedLod >= 0)
  {
    const ezUInt32 uiRequiredLod = static_cast<ezUInt32>(m_iLastRenderedLod);

    // Kick off (or force) generation of the required LOD on the new tree resource.
    pResource->RequestLodMesh(hNewTree, uiSeed, uiRequiredLod, m_bForceGenerateImmediate);

    if (!m_bForceGenerateImmediate)
    {
      // Check whether the LOD is ready yet; if not, retry next frame so the old tree keeps rendering.
      ezResourceLock<ezKrautTreeResource> pNewTree(hNewTree, ezResourceAcquireMode::PointerOnly);
      if (pNewTree.IsValid() && uiRequiredLod < pNewTree->GetTreeLODs().GetCount() &&
          pNewTree->GetLodState(uiRequiredLod) != ezKrautLodState::Ready)
      {
        GetWorld()->GetOrCreateComponentManager<ezKrautTreeComponentManager>()->EnqueueUpdate(GetHandle());
        return;
      }
    }
    // When m_bForceGenerateImmediate, RequestLodMesh generated synchronously — no need to retry.
  }
  else if (m_bForceGenerateImmediate)
  {
    // No previously rendered LOD to wait for. Generate base data + the coarsest runtime LOD
    // synchronously so that bounds are valid within this same frame. This makes first-frame
    // rendering deterministic, which is required for image comparison tests.
    constexpr ezUInt32 uiCoarsestRuntimeLod = 1;
    pResource->RequestLodMesh(hNewTree, uiSeed, uiCoarsestRuntimeLod, true);
  }

  m_uiCurrentSeed = uiSeed;
  m_hKrautTree = hNewTree;
  TriggerLocalBoundsUpdate();
}

void ezKrautTreeComponent::ComputeWind()
{
  if (!IsActiveAndSimulating() || GetOwner()->GetVisibilityState() == ezVisibilityState::Invisible)
    return;

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

    ezTempHybridArray<ezDebugRendererLine, 2> lines;

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
      const ezUInt32 uiSurfIdx = desc.m_Surfaces.IndexOf(details.m_sSurfaceResource);
      if (uiSurfIdx == ezInvalidIndex)
      {
        subMesh.m_uiSurfaceIndex = static_cast<ezUInt16>(desc.m_Surfaces.GetCount());
        desc.m_Surfaces.PushBack(details.m_sSurfaceResource);
      }
      else
      {
        subMesh.m_uiSurfaceIndex = static_cast<ezUInt16>(uiSurfIdx);
      }
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

  {
    auto desc = EZ_CREATE_MODULE_UPDATE_FUNCTION_DESC(ezKrautTreeComponentManager::Update, this);
    desc.m_Phase = ezWorldUpdatePhase::PreAsync;

    RegisterUpdateFunction(desc);
  }

  {
    auto desc = EZ_CREATE_MODULE_UPDATE_FUNCTION_DESC(ezKrautTreeComponentManager::UpdateWind, this);
    desc.m_Phase = ezWorldUpdatePhase::Async;
    desc.m_bOnlyUpdateWhenSimulating = true;
    desc.m_uiAsyncPhaseBatchSize = 16;

    RegisterUpdateFunction(desc);
  }

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

    pComp->EnsureTreeIsGenerated();

    // If the tree resource exists but bounds aren't valid yet (base data task still running),
    // re-enqueue so we trigger a bounds update as soon as the async task completes.
    const ezKrautTreeResourceHandle& hTree = pComp->GetKrautTreeResource();
    if (hTree.IsValid())
    {
      ezResourceLock<ezKrautTreeResource> pTree(hTree, ezResourceAcquireMode::PointerOnly);
      if (pTree.IsValid())
      {
        if (!pTree->GetDetails().m_Bounds.IsValid())
          EnqueueUpdate(hComp);              // retry next frame
        else
          pComp->TriggerLocalBoundsUpdate(); // bounds are now ready; ensure they propagate
      }
    }
  }
}

void ezKrautTreeComponentManager::UpdateWind(const ezWorldModule::UpdateContext& context)
{
  for (auto it = this->m_ComponentStorage.GetIterator(context.m_uiFirstComponentIndex, context.m_uiComponentCount); it.IsValid(); ++it)
  {
    ezKrautTreeComponent* pComponent = it;
    pComponent->ComputeWind();
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
