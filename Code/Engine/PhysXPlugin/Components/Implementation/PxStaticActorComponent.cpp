#include <PhysXPlugin/PCH.h>
#include <PhysXPlugin/Components/PxStaticActorComponent.h>
#include <PhysXPlugin/WorldModule/PhysXWorldModule.h>
#include <PhysXPlugin/WorldModule/Implementation/PhysX.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Core/WorldSerializer/WorldReader.h>

EZ_BEGIN_COMPONENT_TYPE(ezPxStaticActorComponent, 1)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("CollisionMesh", GetMeshFile, SetMeshFile)->AddAttributes(new ezAssetBrowserAttribute("Collision Mesh")),
    EZ_MEMBER_PROPERTY("CollisionLayer", m_uiCollisionLayer)->AddAttributes(new ezDynamicEnumAttribute("PhysicsCollisionLayer")),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

ezPxStaticActorComponent::ezPxStaticActorComponent()
{
  m_pActor = nullptr;
  m_uiCollisionLayer = 0;
}


void ezPxStaticActorComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  auto& s = stream.GetStream();

  s << m_hCollisionMesh;
  s << m_uiCollisionLayer;
}


void ezPxStaticActorComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = stream.GetStream();

  s >> m_hCollisionMesh;
  s >> m_uiCollisionLayer;
}


void ezPxStaticActorComponent::SetMeshFile(const char* szFile)
{
  ezPhysXMeshResourceHandle hMesh;

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


void ezPxStaticActorComponent::SetMesh(const ezPhysXMeshResourceHandle& hMesh)
{
  m_hCollisionMesh = hMesh;
}

void ezPxStaticActorComponent::OnSimulationStarted()
{
  if (!IsActive())
    return;

  ezPhysXWorldModule* pModule = GetWorld()->GetOrCreateModule<ezPhysXWorldModule>();

  const auto pos = GetOwner()->GetGlobalPosition();
  const auto rot = GetOwner()->GetGlobalRotation();

  PxTransform t = PxTransform::createIdentity();
  t.p = PxVec3(pos.x, pos.y, pos.z);
  t.q = PxQuat(rot.v.x, rot.v.y, rot.v.z, rot.w);
  m_pActor = ezPhysX::GetSingleton()->GetPhysXAPI()->createRigidStatic(t);
  EZ_ASSERT_DEBUG(m_pActor != nullptr, "PhysX actor creation failed");

  m_pActor->userData = GetOwner();

  AddShapesFromObject(GetOwner(), m_pActor, GetOwner()->GetGlobalTransform());

  if (m_hCollisionMesh.IsValid())
  {
    ezResourceLock<ezPxMeshResource> pMesh(m_hCollisionMesh);

    ezHybridArray<PxMaterial*, 32> pxMaterials;

    {
      pxMaterials.SetCount(pMesh->GetSurfaces().GetCount());

      for (ezUInt32 mat = 0; mat < pMesh->GetSurfaces().GetCount(); ++mat)
      {
        if (pMesh->GetSurfaces()[mat].IsValid())
        {
          ezResourceLock<ezSurfaceResource> pSurf(pMesh->GetSurfaces()[mat]);

          pxMaterials[mat] = static_cast<PxMaterial*>(pSurf->m_pPhysicsMaterial);
        }
        else
        {
          pxMaterials[mat] = ezPhysX::GetSingleton()->GetDefaultMaterial();
        }
      }
    }

    if (pxMaterials.IsEmpty())
    {
      pxMaterials.PushBack(ezPhysX::GetSingleton()->GetDefaultMaterial());
    }

    if (pMesh->GetTriangleMesh() != nullptr)
    {
      auto pShape = m_pActor->createShape(PxTriangleMeshGeometry(pMesh->GetTriangleMesh()), pxMaterials.GetData(), pxMaterials.GetCount());

      PxFilterData filter;
      filter.word0 = EZ_BIT(m_uiCollisionLayer);
      filter.word1 = ezPhysX::GetSingleton()->GetCollisionFilterConfig().GetFilterMask(m_uiCollisionLayer);
      filter.word2 = 0;
      filter.word3 = 0;
      pShape->setSimulationFilterData(filter);
      pShape->setQueryFilterData(filter);

      pShape->userData = GetOwner();
    }
    else if (pMesh->GetConvexMesh() != nullptr)
    {
      auto pShape = m_pActor->createShape(PxConvexMeshGeometry(pMesh->GetConvexMesh()), pxMaterials.GetData(), pxMaterials.GetCount());

      PxFilterData filter;
      filter.word0 = EZ_BIT(m_uiCollisionLayer);
      filter.word1 = ezPhysX::GetSingleton()->GetCollisionFilterConfig().GetFilterMask(m_uiCollisionLayer);
      filter.word2 = 0;
      filter.word3 = 0;
      pShape->setSimulationFilterData(filter);
      pShape->setQueryFilterData(filter);

      pShape->userData = GetOwner();
    }
    else
    {
      ezLog::Warning("ezPxStaticActorComponent: Collision mesh resource is valid, but it contains no triangle mesh ('{0}' - '{1}')", pMesh->GetResourceID().GetData(), pMesh->GetResourceDescription().GetData());
    }
  }

  // Hacky feature to add a ground plane for static actors that have no shapes at all
  if (m_pActor->getNbShapes() == 0)
  {
    ezQuat qRot;
    qRot.SetFromAxisAndAngle(ezVec3(0, 1, 0), ezAngle::Degree(270));

    auto pShape = m_pActor->createShape(PxPlaneGeometry(), *ezPhysX::GetSingleton()->GetDefaultMaterial());
    pShape->setLocalPose(PxTransform(PxQuat(qRot.v.x, qRot.v.y, qRot.v.z, qRot.w)));

    PxFilterData filter;
    filter.word0 = EZ_BIT(m_uiCollisionLayer);
    filter.word1 = ezPhysX::GetSingleton()->GetCollisionFilterConfig().GetFilterMask(m_uiCollisionLayer);
    filter.word2 = 0;
    filter.word3 = 0;
    pShape->setSimulationFilterData(filter);
    pShape->setQueryFilterData(filter);

    pShape->userData = GetOwner();
  }

  {
    EZ_PX_WRITE_LOCK(*(pModule->GetPxScene()));
    pModule->GetPxScene()->addActor(*m_pActor);
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
}

