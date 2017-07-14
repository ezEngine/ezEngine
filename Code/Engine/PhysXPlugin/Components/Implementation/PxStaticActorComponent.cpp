#include <PCH.h>
#include <PhysXPlugin/Components/PxStaticActorComponent.h>
#include <PhysXPlugin/WorldModule/PhysXWorldModule.h>
#include <PhysXPlugin/WorldModule/Implementation/PhysX.h>
#include <PhysXPlugin/Utilities/PxConversionUtils.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <GameEngine/Messages/BuildNavMeshMessage.h>

EZ_BEGIN_COMPONENT_TYPE(ezPxStaticActorComponent, 2)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("CollisionMesh", GetMeshFile, SetMeshFile)->AddAttributes(new ezAssetBrowserAttribute("Collision Mesh;Collision Mesh (Convex)")),
    EZ_MEMBER_PROPERTY("CollisionLayer", m_uiCollisionLayer)->AddAttributes(new ezDynamicEnumAttribute("PhysicsCollisionLayer")),
    EZ_MEMBER_PROPERTY("IncludeInNavmesh", m_bIncludeInNavmesh)->AddAttributes(new ezDefaultValueAttribute(true)),
  }
  EZ_END_PROPERTIES
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezBuildNavMeshMessage, OnBuildNavMesh),
  }
  EZ_END_MESSAGEHANDLERS
}
EZ_END_DYNAMIC_REFLECTED_TYPE

ezPxStaticActorComponent::ezPxStaticActorComponent()
  : m_uiCollisionLayer(0)
  , m_uiShapeId(ezInvalidIndex)
  , m_pActor(nullptr)
  , m_UserData(this)
{
}

ezPxStaticActorComponent::~ezPxStaticActorComponent()
{
}

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

void ezPxStaticActorComponent::Deinitialize()
{
  if (m_pActor != nullptr)
  {
    EZ_PX_WRITE_LOCK(*(m_pActor->getScene()));

    m_pActor->release();
    m_pActor = nullptr;
  }

  if (m_uiShapeId != ezInvalidIndex)
  {
    if (ezPhysXWorldModule* pModule = GetWorld()->GetModule<ezPhysXWorldModule>())
    {
      pModule->DeleteShapeId(m_uiShapeId);
      m_uiShapeId = ezInvalidIndex;
    }
  }
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

  m_pActor->userData = &m_UserData;

  // PhysX does not get any scale value, so to correctly position child objects
  // we have to pretend that this parent object applies no scale on its children
  ezSimdTransform globalTransformNoScale = globalTransform;
  globalTransformNoScale.m_Scale.Set(1.0f);
  AddShapesFromObject(GetOwner(), m_pActor, globalTransformNoScale);

  PxShape* pShape = nullptr;

  if (m_hCollisionMesh.IsValid())
  {
    m_uiShapeId = pModule->CreateShapeId();

    ezResourceLock<ezPxMeshResource> pMesh(m_hCollisionMesh);

    ezHybridArray<PxMaterial*, 32> pxMaterials;

    {
      const auto& surfaces = pMesh->GetSurfaces();
      pxMaterials.SetCountUninitialized(surfaces.GetCount());

      for (ezUInt32 i = 0; i < surfaces.GetCount(); ++i)
      {
        if (surfaces[i].IsValid())
        {
          ezResourceLock<ezSurfaceResource> pSurface(surfaces[i]);
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

    PxMeshScale scale = ezPxConversionUtils::ToScale(globalTransform);

    if (pMesh->GetTriangleMesh() != nullptr)
    {
      pShape = m_pActor->createShape(PxTriangleMeshGeometry(pMesh->GetTriangleMesh(), scale), pxMaterials.GetData(), pxMaterials.GetCount());
    }
    else if (pMesh->GetConvexMesh() != nullptr)
    {
      pShape = m_pActor->createShape(PxConvexMeshGeometry(pMesh->GetConvexMesh(), scale), pxMaterials.GetData(), pxMaterials.GetCount());
    }
    else
    {
      ezLog::Warning("ezPxStaticActorComponent: Collision mesh resource is valid, but it contains no triangle mesh ('{0}' - '{1}')", pMesh->GetResourceID(), pMesh->GetResourceDescription());
    }
  }

  // Hacky feature to add a ground plane for static actors that have no shapes at all
  if (m_pActor->getNbShapes() == 0)
  {
    ezLog::Error("Static Physics Actor component without shape is used.");

    //pShape = m_pActor->createShape(PxPlaneGeometry(), *ezPhysX::GetSingleton()->GetDefaultMaterial());
    //pShape->setLocalPose(PxTransform(PxQuat(ezAngle::Degree(270.0f).GetRadian(), PxVec3(0.0f, 1.0f, 0.0f))));

    return;
  }

  if (pShape != nullptr)
  {
    PxFilterData filterData = ezPhysX::CreateFilterData(m_uiCollisionLayer, m_uiShapeId);

    pShape->setSimulationFilterData(filterData);
    pShape->setQueryFilterData(filterData);

    pShape->userData = &m_UserData;
  }

  {
    EZ_PX_WRITE_LOCK(*(pModule->GetPxScene()));
    pModule->GetPxScene()->addActor(*m_pActor);
  }
}

void ezPxStaticActorComponent::OnBuildNavMesh(ezBuildNavMeshMessage& msg) const
{
  if (!m_bIncludeInNavmesh)
    return;

  if (m_hCollisionMesh.IsValid())
  {
    ezResourceLock<ezPxMeshResource> pMesh(m_hCollisionMesh, ezResourceAcquireMode::NoFallback);

    pMesh->AddToNavMesh(GetOwner()->GetGlobalTransform(), msg);
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

