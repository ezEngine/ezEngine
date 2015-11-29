#include <PhysXPlugin/PCH.h>
#include <PhysXPlugin/Components/PxStaticActorComponent.h>
#include <PhysXPlugin/PhysXSceneModule.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Core/WorldSerializer/WorldReader.h>

EZ_BEGIN_COMPONENT_TYPE(ezPxStaticActorComponent, 1);
  EZ_BEGIN_PROPERTIES
    EZ_ACCESSOR_PROPERTY("Collision Mesh", GetMeshFile, SetMeshFile)->AddAttributes(new ezAssetBrowserAttribute("Collision Mesh")),
  EZ_END_PROPERTIES
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezPxStaticActorComponent::ezPxStaticActorComponent()
{
  m_pActor = nullptr;
}


void ezPxStaticActorComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  auto& s = stream.GetStream();

  /// \todo Serialize resource handles more efficiently
  s << GetMeshFile();
}


void ezPxStaticActorComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);

  auto& s = stream.GetStream();

  ezStringBuilder sTemp;
  s >> sTemp;
  SetMeshFile(sTemp);
}


void ezPxStaticActorComponent::SetMeshFile(const char* szFile)
{
  ezPhysXMeshResourceHandle hMesh;

  if (!ezStringUtils::IsNullOrEmpty(szFile))
  {
    hMesh = ezResourceManager::LoadResource<ezPhysXMeshResource>(szFile);
  }

  SetMesh(hMesh);
}


const char* ezPxStaticActorComponent::GetMeshFile() const
{
  if (!m_hCollisionMesh.IsValid())
    return "";

  ezResourceLock<ezPhysXMeshResource> pMesh(m_hCollisionMesh);
  return pMesh->GetResourceID();
}


void ezPxStaticActorComponent::SetMesh(const ezPhysXMeshResourceHandle& hMesh)
{
  m_hCollisionMesh = hMesh;

  if (m_hCollisionMesh.IsValid())
    ezResourceManager::PreloadResource(m_hCollisionMesh, ezTime::Seconds(5.0));
}

ezComponent::Initialization ezPxStaticActorComponent::Initialize()
{
  ezPhysXSceneModule* pModule = static_cast<ezPhysXSceneModule*>(GetManager()->GetUserData());

  const auto pos = GetOwner()->GetGlobalPosition();
  const auto rot = GetOwner()->GetGlobalRotation();

  PxTransform t = PxTransform::createIdentity();
  t.p = PxVec3(pos.x, pos.y, pos.z);
  t.q = PxQuat(rot.v.x, rot.v.y, rot.v.z, rot.w);
  m_pActor = ezPhysX::GetSingleton()->GetPhysXAPI()->createRigidStatic(t);
  EZ_ASSERT_DEBUG(m_pActor != nullptr, "PhysX actor creation failed");

  AddShapesFromObject(GetOwner(), m_pActor, GetOwner()->GetGlobalTransform());
  AddShapesFromChildren(GetOwner(), m_pActor, GetOwner()->GetGlobalTransform());

  if (m_hCollisionMesh.IsValid())
  {
    ezResourceLock<ezPhysXMeshResource> pMesh(m_hCollisionMesh);

    auto pTriMesh = pMesh->GetTriangleMesh();

    if (pTriMesh != nullptr)
    {
      /// \todo Material(s)
      m_pActor->createShape(PxTriangleMeshGeometry(pTriMesh), *ezPhysX::GetSingleton()->GetDefaultMaterial());
    }
    else
    {
      ezLog::Warning("ezPxStaticActorComponent: Collision mesh resource is valid, but it contains no triangle mesh ('%s' - '%s')", pMesh->GetResourceID().GetData(), pMesh->GetResourceDescription().GetData());
    }
  }

  if (m_pActor->getNbShapes() == 0)
  {
    ezQuat qRot;
    qRot.SetFromAxisAndAngle(ezVec3(0, 1, 0), ezAngle::Degree(270));

    auto pShape = m_pActor->createShape(PxPlaneGeometry(), *ezPhysX::GetSingleton()->GetDefaultMaterial());
    pShape->setLocalPose(PxTransform(PxQuat(qRot.v.x, qRot.v.y, qRot.v.z, qRot.w)));
  }

  pModule->GetPxScene()->addActor(*m_pActor);

  return ezComponent::Initialization::Done;
}

void ezPxStaticActorComponent::Deinitialize()
{
  ezPhysXSceneModule* pModule = static_cast<ezPhysXSceneModule*>(GetManager()->GetUserData());

  m_pActor->release();
  m_pActor = nullptr;
}

