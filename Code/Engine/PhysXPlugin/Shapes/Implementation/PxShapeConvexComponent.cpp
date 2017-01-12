#include <PhysXPlugin/PCH.h>
#include <PhysXPlugin/Shapes/PxShapeConvexComponent.h>
#include <PhysXPlugin/WorldModule/PhysXWorldModule.h>
#include <PhysXPlugin/WorldModule/Implementation/PhysX.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Core/WorldSerializer/WorldReader.h>

EZ_BEGIN_COMPONENT_TYPE(ezPxShapeConvexComponent, 1)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("CollisionMesh", GetMeshFile, SetMeshFile)->AddAttributes(new ezAssetBrowserAttribute("Collision Mesh (Convex)")),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

ezPxShapeConvexComponent::ezPxShapeConvexComponent()
{
}


void ezPxShapeConvexComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  auto& s = stream.GetStream();

  s << m_hCollisionMesh;
}


void ezPxShapeConvexComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = stream.GetStream();

  s >> m_hCollisionMesh;
}

void ezPxShapeConvexComponent::SetMeshFile(const char* szFile)
{
  if (!ezStringUtils::IsNullOrEmpty(szFile))
  {
    m_hCollisionMesh = ezResourceManager::LoadResource<ezPxMeshResource>(szFile);
  }
}


const char* ezPxShapeConvexComponent::GetMeshFile() const
{
  if (!m_hCollisionMesh.IsValid())
    return "";

  return m_hCollisionMesh.GetResourceID();
}


void ezPxShapeConvexComponent::AddToActor(PxRigidActor* pActor, const ezTransform& ParentTransform)
{
  if (!m_hCollisionMesh.IsValid())
  {
    ezLog::Warning("ezPxShapeConvexComponent '{0}' has no collision mesh set.", GetOwner()->GetName());
    return;
  }

  ezResourceLock<ezPxMeshResource> pMesh(m_hCollisionMesh);

  if (!pMesh->GetConvexMesh())
  {
    ezLog::Warning("ezPxShapeConvexComponent '{0}' has a collision mesh set that does not contain a convex mesh: '{1}' ('{2}')", GetOwner()->GetName(), pMesh->GetResourceID().GetData(), pMesh->GetResourceDescription().GetData());
    return;
  }

  ezPhysXWorldModule* pModule = GetWorld()->GetOrCreateModule<ezPhysXWorldModule>();

  const ezTransform OwnerTransform = GetOwner()->GetGlobalTransform();

  ezTransform LocalTransform;
  LocalTransform.SetLocalTransform(ParentTransform, OwnerTransform);

  ezQuat r;
  r.SetFromMat3(LocalTransform.m_Rotation);

  PxTransform t;
  t.p = PxVec3(LocalTransform.m_vPosition.x, LocalTransform.m_vPosition.y, LocalTransform.m_vPosition.z);
  t.q = PxQuat(r.v.x, r.v.y, r.v.z, r.w);

  PxMaterial* pMaterial = nullptr;

  if (m_hSurface.IsValid())
    pMaterial = GetPxMaterial();
  else
  {
    ezResourceLock<ezPxMeshResource> pMesh(m_hCollisionMesh);
    const auto& surfs = pMesh->GetSurfaces();

    if (!surfs.IsEmpty())
    {
      ezResourceLock<ezSurfaceResource> pSurface(surfs[0]);
      pMaterial = static_cast<PxMaterial*>(pSurface->m_pPhysicsMaterial);
    }
  }

  if (pMaterial == nullptr)
    pMaterial = ezPhysX::GetSingleton()->GetDefaultMaterial();

  auto pShape = pActor->createShape(PxConvexMeshGeometry(pMesh->GetConvexMesh()), *pMaterial);
  pShape->setLocalPose(t);

  EZ_ASSERT_DEBUG(pShape != nullptr, "PhysX convex shape creation failed");

  PxFilterData filter = CreateFilterData();
  pShape->setSimulationFilterData(filter);
  pShape->setQueryFilterData(filter);

  pShape->userData = &m_UserData;
}

