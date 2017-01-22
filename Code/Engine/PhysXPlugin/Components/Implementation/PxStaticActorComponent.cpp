#include <PhysXPlugin/PCH.h>
#include <PhysXPlugin/Components/PxStaticActorComponent.h>
#include <PhysXPlugin/WorldModule/PhysXWorldModule.h>
#include <PhysXPlugin/WorldModule/Implementation/PhysX.h>
#include <PhysXPlugin/Utilities/PxConversionUtils.h>
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
}


void ezPxStaticActorComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = stream.GetStream();

  s >> m_hCollisionMesh;
  s >> m_uiCollisionLayer;
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

  PxTransform t = ezPxConversionUtils::ToTransform(GetOwner()->GetGlobalTransform());
  m_pActor = ezPhysX::GetSingleton()->GetPhysXAPI()->createRigidStatic(t);
  EZ_ASSERT_DEBUG(m_pActor != nullptr, "PhysX actor creation failed");

  m_pActor->userData = &m_UserData;

  AddShapesFromObject(GetOwner(), m_pActor, GetOwner()->GetGlobalTransform());

  PxShape* pShape = nullptr;

  if (m_hCollisionMesh.IsValid())
  {
    m_uiShapeId = pModule->CreateShapeId();

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
      pShape = m_pActor->createShape(PxTriangleMeshGeometry(pMesh->GetTriangleMesh()), pxMaterials.GetData(), pxMaterials.GetCount());
    }
    else if (pMesh->GetConvexMesh() != nullptr)
    {
      pShape = m_pActor->createShape(PxConvexMeshGeometry(pMesh->GetConvexMesh()), pxMaterials.GetData(), pxMaterials.GetCount());
    }
    else
    {
      ezLog::Warning("ezPxStaticActorComponent: Collision mesh resource is valid, but it contains no triangle mesh ('{0}' - '{1}')", pMesh->GetResourceID().GetData(), pMesh->GetResourceDescription().GetData());
    }
  }

  // Hacky feature to add a ground plane for static actors that have no shapes at all
  if (m_pActor->getNbShapes() == 0)
  {
    pShape = m_pActor->createShape(PxPlaneGeometry(), *ezPhysX::GetSingleton()->GetDefaultMaterial());
    pShape->setLocalPose(PxTransform(PxQuat(ezAngle::Degree(270.0f).GetRadian(), PxVec3(0.0f, 1.0f, 0.0f))));
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
