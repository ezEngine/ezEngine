#include <PhysXPluginPCH.h>

#include <Core/Utils/WorldGeoExtractionUtil.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <PhysXPlugin/Components/PxStaticActorComponent.h>
#include <PhysXPlugin/Utilities/PxConversionUtils.h>
#include <PhysXPlugin/WorldModule/Implementation/PhysX.h>
#include <PhysXPlugin/WorldModule/PhysXWorldModule.h>
#include <RendererCore/Meshes/MeshComponent.h>
#include <extensions/PxRigidActorExt.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezPxStaticActorComponent, 2, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("CollisionMesh", GetMeshFile, SetMeshFile)->AddAttributes(new ezAssetBrowserAttribute("Collision Mesh;Collision Mesh (Convex)")),
    EZ_MEMBER_PROPERTY("CollisionLayer", m_uiCollisionLayer)->AddAttributes(new ezDynamicEnumAttribute("PhysicsCollisionLayer")),
    EZ_MEMBER_PROPERTY("IncludeInNavmesh", m_bIncludeInNavmesh)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_MEMBER_PROPERTY("PullSurfacesFromGraphicsMesh", m_bPullSurfacesFromGraphicsMesh)->AddAttributes(new ezDefaultValueAttribute(true)),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgExtractGeometry, OnMsgExtractGeometry),
  }
  EZ_END_MESSAGEHANDLERS;
  EZ_BEGIN_FUNCTIONS
  {
    EZ_SCRIPT_FUNCTION_PROPERTY(GetShapeId),
  }
  EZ_END_FUNCTIONS;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezPxStaticActorComponent::ezPxStaticActorComponent() = default;
ezPxStaticActorComponent::~ezPxStaticActorComponent() = default;

void ezPxStaticActorComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  auto& s = stream.GetStream();

  s << m_hCollisionMesh;
  s << m_uiCollisionLayer;
  s << m_bIncludeInNavmesh;
}


void ezPxStaticActorComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = stream.GetStream();

  s >> m_hCollisionMesh;
  s >> m_uiCollisionLayer;

  if (uiVersion >= 2)
  {
    s >> m_bIncludeInNavmesh;
  }
}

void ezPxStaticActorComponent::OnDeactivated()
{
  if (m_pActor != nullptr)
  {
    ezPhysXWorldModule* pModule = GetWorld()->GetModule<ezPhysXWorldModule>();

    if (pModule->GetPxScene() != nullptr)
    {
      EZ_PX_WRITE_LOCK(*(pModule->GetPxScene()));

      m_pActor->release();
    }

    m_pActor = nullptr;

    pModule->DeleteShapeId(m_uiShapeId);
    pModule->DeallocateUserData(m_uiUserDataIndex);
  }

  m_UsedSurfaces.Clear();

  SUPER::OnDeactivated();
}

void ezPxStaticActorComponent::OnSimulationStarted()
{
  if (!IsActive())
    return;

  ezPhysXWorldModule* pModule = GetWorld()->GetOrCreateModule<ezPhysXWorldModule>();

  const ezSimdTransform& globalTransform = GetOwner()->GetGlobalTransformSimd();

  PxTransform t = ezPxConversionUtils::ToTransform(globalTransform);
  m_pActor = ezPhysX::GetSingleton()->GetPhysXAPI()->createRigidStatic(t);
  EZ_ASSERT_DEBUG(m_pActor != nullptr, "PhysX actor creation failed");

  ezPxUserData* pUserData = nullptr;
  m_uiUserDataIndex = pModule->AllocateUserData(pUserData);
  pUserData->Init(this);
  m_pActor->userData = pUserData;

  // PhysX does not get any scale value, so to correctly position child objects
  // we have to pretend that this parent object applies no scale on its children
  ezSimdTransform globalTransformNoScale = globalTransform;
  globalTransformNoScale.m_Scale.Set(1.0f);
  AddShapesFromObject(GetOwner(), m_pActor, globalTransformNoScale);

  ezHybridArray<PxShape*, 4> shapes;

  if (m_hCollisionMesh.IsValid())
  {
    m_uiShapeId = pModule->CreateShapeId();

    ezResourceLock<ezPxMeshResource> pMesh(m_hCollisionMesh, ezResourceAcquireMode::BlockTillLoaded);

    ezHybridArray<PxMaterial*, 32> pxMaterials;

    {
      const auto& surfaces = pMesh->GetSurfaces();
      pxMaterials.SetCountUninitialized(surfaces.GetCount());

      for (ezUInt32 i = 0; i < surfaces.GetCount(); ++i)
      {
        if (surfaces[i].IsValid())
        {
          ezResourceLock<ezSurfaceResource> pSurface(surfaces[i], ezResourceAcquireMode::BlockTillLoaded);
          if (pSurface)
          {
            pxMaterials[i] = static_cast<PxMaterial*>(pSurface->m_pPhysicsMaterial);

            continue;
          }
        }

        pxMaterials[i] = ezPhysX::GetSingleton()->GetDefaultMaterial();
      }
    }

    if (pxMaterials.IsEmpty())
    {
      pxMaterials.PushBack(ezPhysX::GetSingleton()->GetDefaultMaterial());
    }

    if (m_bPullSurfacesFromGraphicsMesh)
    {
      PullSurfacesFromGraphicsMesh(pxMaterials);
    }

    PxMeshScale scale = ezPxConversionUtils::ToScale(globalTransform);

    if (pMesh->GetTriangleMesh() != nullptr)
    {
      shapes.PushBack(PxRigidActorExt::createExclusiveShape(*m_pActor, PxTriangleMeshGeometry(pMesh->GetTriangleMesh(), scale), pxMaterials.GetData(), pxMaterials.GetCount()));
    }
    else if (!pMesh->GetConvexParts().IsEmpty())
    {
      for (auto pShape : pMesh->GetConvexParts())
      {
        shapes.PushBack(PxRigidActorExt::createExclusiveShape(*m_pActor, PxConvexMeshGeometry(pShape, scale), pxMaterials.GetData(), pxMaterials.GetCount()));
      }
    }
    else
    {
      ezLog::Warning("ezPxStaticActorComponent: Collision mesh resource is valid, but it contains no triangle mesh ('{0}' - '{1}')", pMesh->GetResourceID(), pMesh->GetResourceDescription());
    }
  }

  if (m_pActor->getNbShapes() == 0)
  {
    ezLog::Error("Static Physics Actor component without shape is used.");
    return;
  }

  if (!shapes.IsEmpty())
  {
    PxFilterData filterData = ezPhysX::CreateFilterData(m_uiCollisionLayer, m_uiShapeId);

    for (auto pShape : shapes)
    {
      pShape->setSimulationFilterData(filterData);
      pShape->setQueryFilterData(filterData);

      pShape->userData = pUserData;
    }
  }

  {
    EZ_PX_WRITE_LOCK(*(pModule->GetPxScene()));
    pModule->GetPxScene()->addActor(*m_pActor);
  }
}

void ezPxStaticActorComponent::PullSurfacesFromGraphicsMesh(ezHybridArray<physx::PxMaterial*, 32>& pxMaterials)
{
  // the materials don't hold a handle to the surfaces, so they don't keep them alive
  // therefore, we need to keep them alive by storing a handle
  m_UsedSurfaces.Clear();

  ezMeshComponent* pMeshComp;
  if (!GetOwner()->TryGetComponentOfBaseType(pMeshComp))
    return;

  auto hMeshRes = pMeshComp->GetMesh();
  if (!hMeshRes.IsValid())
    return;

  ezResourceLock<ezMeshResource> pMeshRes(hMeshRes, ezResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (pMeshRes.GetAcquireResult() != ezResourceAcquireResult::Final)
    return;

  if (pMeshRes->GetMaterials().GetCount() != pxMaterials.GetCount())
    return;

  const ezUInt32 uiNumMats = pxMaterials.GetCount();
  m_UsedSurfaces.SetCount(uiNumMats);

  for (ezUInt32 s = 0; s < uiNumMats; ++s)
  {
    // first check whether the component has a material override
    auto hMat = pMeshComp->GetMaterial(s);

    if (!hMat.IsValid())
    {
      // otherwise ask the mesh resource about the material
      hMat = pMeshRes->GetMaterials()[s];
    }

    if (!hMat.IsValid())
      continue;

    ezResourceLock<ezMaterialResource> pMat(hMat, ezResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (pMat.GetAcquireResult() != ezResourceAcquireResult::Final)
      continue;

    if (pMat->GetSurface().IsEmpty())
      continue;

    m_UsedSurfaces[s] = ezResourceManager::LoadResource<ezSurfaceResource>(pMat->GetSurface().GetString());

    ezResourceLock<ezSurfaceResource> pSurface(m_UsedSurfaces[s], ezResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (pSurface.GetAcquireResult() != ezResourceAcquireResult::Final)
      continue;

    EZ_ASSERT_DEV(pSurface->m_pPhysicsMaterial != nullptr, "Invalid PhysX material pointer on surface");
    pxMaterials[s] = static_cast<PxMaterial*>(pSurface->m_pPhysicsMaterial);
  }
}

void ezPxStaticActorComponent::OnMsgExtractGeometry(ezMsgExtractGeometry& msg) const
{
  if (!m_bIncludeInNavmesh)
    return;

  if (msg.m_Mode != ezWorldGeoExtractionUtil::ExtractionMode::CollisionMesh && msg.m_Mode != ezWorldGeoExtractionUtil::ExtractionMode::NavMeshGeneration)
    return;

  if (m_hCollisionMesh.IsValid())
  {
    ezResourceLock<ezPxMeshResource> pMesh(m_hCollisionMesh, ezResourceAcquireMode::BlockTillLoaded);

    pMesh->ExtractGeometry(GetOwner()->GetGlobalTransform(), msg);
  }

  AddShapesToNavMesh(GetOwner(), msg);
}

void ezPxStaticActorComponent::SetMeshFile(const char* szFile)
{
  ezPxMeshResourceHandle hMesh;

  if (!ezStringUtils::IsNullOrEmpty(szFile))
  {
    hMesh = ezResourceManager::LoadResource<ezPxMeshResource>(szFile);
  }

  SetMesh(hMesh);
}

const char* ezPxStaticActorComponent::GetMeshFile() const
{
  if (!m_hCollisionMesh.IsValid())
    return "";

  return m_hCollisionMesh.GetResourceID();
}

void ezPxStaticActorComponent::SetMesh(const ezPxMeshResourceHandle& hMesh)
{
  m_hCollisionMesh = hMesh;
}

EZ_STATICLINK_FILE(PhysXPlugin, PhysXPlugin_Components_Implementation_PxStaticActorComponent);
