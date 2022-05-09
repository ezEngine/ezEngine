#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyID.h>
#include <Jolt/Physics/Collision/Shape/ConvexHullShape.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <JoltPlugin/Actors/JoltStaticActorComponent.h>
#include <JoltPlugin/Resources/JoltMaterial.h>
#include <JoltPlugin/Resources/JoltMeshResource.h>
#include <JoltPlugin/System/JoltCollisionFiltering.h>
#include <JoltPlugin/System/JoltWorldModule.h>
#include <JoltPlugin/Utilities/JoltConversionUtils.h>
#include <RendererCore/Meshes/MeshComponent.h>
#include <RendererCore/Utils/WorldGeoExtractionUtil.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezJoltStaticActorComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("CollisionMesh", GetMeshFile, SetMeshFile)->AddAttributes(new ezAssetBrowserAttribute("Jolt_Colmesh_Triangle;Jolt_Colmesh_Convex")),
    EZ_MEMBER_PROPERTY("IncludeInNavmesh", m_bIncludeInNavmesh)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_MEMBER_PROPERTY("PullSurfacesFromGraphicsMesh", m_bPullSurfacesFromGraphicsMesh),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgExtractGeometry, OnMsgExtractGeometry),
  }
  EZ_END_MESSAGEHANDLERS;
  EZ_BEGIN_FUNCTIONS
  {
    EZ_SCRIPT_FUNCTION_PROPERTY(GetObjectFilterID),
  }
  EZ_END_FUNCTIONS;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezJoltStaticActorComponent::ezJoltStaticActorComponent()
{
  m_uiJoltBodyID = JPH::BodyID::cInvalidBodyID;
}

ezJoltStaticActorComponent::~ezJoltStaticActorComponent() = default;

void ezJoltStaticActorComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  auto& s = stream.GetStream();

  s << m_hCollisionMesh;
  s << m_bIncludeInNavmesh;
  s << m_bPullSurfacesFromGraphicsMesh;
}


void ezJoltStaticActorComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = stream.GetStream();

  s >> m_hCollisionMesh;
  s >> m_bIncludeInNavmesh;
  s >> m_bPullSurfacesFromGraphicsMesh;
}

void ezJoltStaticActorComponent::OnDeactivated()
{
  m_UsedSurfaces.Clear();

  SUPER::OnDeactivated();
}

void ezJoltStaticActorComponent::OnSimulationStarted()
{
  ezJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<ezJoltWorldModule>();

  ezJoltUserData* pUserData = nullptr;
  m_uiUserDataIndex = pModule->AllocateUserData(pUserData);
  pUserData->Init(this);

  JPH::BodyCreationSettings bodyCfg;
  if (CreateShape(&bodyCfg, 1.0f).Failed())
  {
    ezLog::Error("Jolt static actor component '{}' has no valid shape.", GetOwner()->GetName());
    return;
  }

  const ezSimdTransform trans = GetOwner()->GetGlobalTransformSimd();

  auto* pSystem = pModule->GetJoltSystem();
  auto* pBodies = &pSystem->GetBodyInterface();
  auto* pMaterial = GetJoltMaterial();

  if (pMaterial == nullptr)
    pMaterial = ezJoltCore::GetDefaultMaterial();

  bodyCfg.mPosition = ezJoltConversionUtils::ToVec3(trans.m_Position);
  bodyCfg.mRotation = ezJoltConversionUtils::ToQuat(trans.m_Rotation);
  bodyCfg.mMotionType = JPH::EMotionType::Static;
  bodyCfg.mObjectLayer = ezJoltCollisionFiltering::ConstructObjectLayer(m_uiCollisionLayer, ezJoltBroadphaseLayer::Static);
  bodyCfg.mRestitution = pMaterial->m_fRestitution;
  bodyCfg.mFriction = pMaterial->m_fFriction;
  bodyCfg.mCollisionGroup.SetGroupID(m_uiObjectFilterID);
  bodyCfg.mCollisionGroup.SetGroupFilter(pModule->GetGroupFilter());
  bodyCfg.mUserData = reinterpret_cast<ezUInt64>(pUserData);

  JPH::Body* pBody = pBodies->CreateBody(bodyCfg);
  m_uiJoltBodyID = pBody->GetID().GetIndexAndSequenceNumber();

  pModule->QueueBodyToAdd(pBody);
}

void ezJoltStaticActorComponent::CreateShapes(ezDynamicArray<ezJoltSubShape>& out_Shapes, const ezTransform& rootTransform, float fDensity, const ezJoltMaterial* pMaterial)
{
  if (!m_hCollisionMesh.IsValid())
    return;

  ezResourceLock<ezJoltMeshResource> pMesh(m_hCollisionMesh, ezResourceAcquireMode::BlockTillLoaded);

  if (pMesh->GetNumConvexParts() > 0)
  {
    for (ezUInt32 i = 0; i < pMesh->GetNumConvexParts(); ++i)
    {
      auto pShape = pMesh->InstantiateConvexPart(i, reinterpret_cast<ezUInt64>(GetUserData()), pMaterial, fDensity);

      ezJoltSubShape& sub = out_Shapes.ExpandAndGetRef();
      sub.m_pShape = pShape;
      sub.m_Transform.SetLocalTransform(rootTransform, GetOwner()->GetGlobalTransform());
    }
  }

  if (auto pTriMesh = pMesh->HasTriangleMesh())
  {
    ezHybridArray<const ezJoltMaterial*, 32> materials;

    if (pMaterial != nullptr)
    {
      materials.SetCount(pMesh->GetSurfaces().GetCount(), pMaterial);
    }

    if (m_bPullSurfacesFromGraphicsMesh)
    {
      materials.SetCount(pMesh->GetSurfaces().GetCount());
      PullSurfacesFromGraphicsMesh(materials);
    }

    auto pNewShape = pMesh->InstantiateTriangleMesh(reinterpret_cast<ezUInt64>(GetUserData()), materials);

    ezJoltSubShape& sub = out_Shapes.ExpandAndGetRef();
    sub.m_pShape = pNewShape;
    sub.m_Transform.SetLocalTransform(rootTransform, GetOwner()->GetGlobalTransform());
  }
}

void ezJoltStaticActorComponent::PullSurfacesFromGraphicsMesh(ezDynamicArray<const ezJoltMaterial*>& materials)
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

  if (pMeshRes->GetMaterials().GetCount() != materials.GetCount())
    return;

  const ezUInt32 uiNumMats = materials.GetCount();
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

    EZ_ASSERT_DEV(pSurface->m_pPhysicsMaterialJolt != nullptr, "Invalid Jolt material pointer on surface");
    materials[s] = static_cast<ezJoltMaterial*>(pSurface->m_pPhysicsMaterialJolt);
  }
}

void ezJoltStaticActorComponent::OnMsgExtractGeometry(ezMsgExtractGeometry& msg) const
{
  if (msg.m_Mode == ezWorldGeoExtractionUtil::ExtractionMode::CollisionMesh || (msg.m_Mode == ezWorldGeoExtractionUtil::ExtractionMode::NavMeshGeneration && m_bIncludeInNavmesh))
  {
    if (m_hCollisionMesh.IsValid())
    {
      ezResourceLock<ezJoltMeshResource> pMesh(m_hCollisionMesh, ezResourceAcquireMode::BlockTillLoaded);

      msg.AddMeshObject(GetOwner()->GetGlobalTransform(), pMesh->ConvertToCpuMesh());
    }

    ExtractSubShapeGeometry(GetOwner(), msg);
  }
}

void ezJoltStaticActorComponent::SetMeshFile(const char* szFile)
{
  ezJoltMeshResourceHandle hMesh;

  if (!ezStringUtils::IsNullOrEmpty(szFile))
  {
    hMesh = ezResourceManager::LoadResource<ezJoltMeshResource>(szFile);
  }

  SetMesh(hMesh);
}

const char* ezJoltStaticActorComponent::GetMeshFile() const
{
  if (!m_hCollisionMesh.IsValid())
    return "";

  return m_hCollisionMesh.GetResourceID();
}

void ezJoltStaticActorComponent::SetMesh(const ezJoltMeshResourceHandle& hMesh)
{
  m_hCollisionMesh = hMesh;
}
