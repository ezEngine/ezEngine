#include <PhysXPluginPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <PhysXPlugin/Shapes/PxShapeConvexComponent.h>
#include <PhysXPlugin/Utilities/PxConversionUtils.h>
#include <PhysXPlugin/WorldModule/Implementation/PhysX.h>
#include <Core/Utils/WorldGeoExtractionUtil.h>

using namespace physx;

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezPxShapeConvexComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("CollisionMesh", GetMeshFile, SetMeshFile)->AddAttributes(new ezAssetBrowserAttribute("Collision Mesh (Convex)")),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezPxShapeConvexComponent::ezPxShapeConvexComponent() {}


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


PxShape* ezPxShapeConvexComponent::CreateShape(PxRigidActor* pActor, PxTransform& out_ShapeTransform)
{
  if (!m_hCollisionMesh.IsValid())
  {
    ezLog::Warning("ezPxShapeConvexComponent '{0}' has no collision mesh set.", GetOwner()->GetName());
    return nullptr;
  }

  ezResourceLock<ezPxMeshResource> pMesh(m_hCollisionMesh, ezResourceAcquireMode::AllowLoadingFallback);

  if (!pMesh->GetConvexMesh())
  {
    ezLog::Warning("ezPxShapeConvexComponent '{0}' has a collision mesh set that does not contain a convex mesh: '{1}' ('{2}')",
                   GetOwner()->GetName(), pMesh->GetResourceID(), pMesh->GetResourceDescription());
    return nullptr;
  }

  PxMaterial* pMaterial = nullptr;

  if (m_hSurface.IsValid())
  {
    pMaterial = GetPxMaterial();
  }
  else
  {
    const auto& surfaces = pMesh->GetSurfaces();

    if (!surfaces.IsEmpty() && surfaces[0].IsValid())
    {
      ezResourceLock<ezSurfaceResource> pSurface(surfaces[0], ezResourceAcquireMode::AllowLoadingFallback);
      pMaterial = static_cast<PxMaterial*>(pSurface->m_pPhysicsMaterial);
    }
  }

  if (pMaterial == nullptr)
  {
    pMaterial = ezPhysX::GetSingleton()->GetDefaultMaterial();
  }

  PxMeshScale scale = ezPxConversionUtils::ToScale(GetOwner()->GetGlobalTransformSimd());

  return pActor->createShape(PxConvexMeshGeometry(pMesh->GetConvexMesh(), scale), *pMaterial);
}

void ezPxShapeConvexComponent::ExtractGeometry(ezMsgExtractGeometry& msg) const
{
  if (msg.m_Mode != ezWorldGeoExtractionUtil::ExtractionMode::CollisionMesh &&
      msg.m_Mode != ezWorldGeoExtractionUtil::ExtractionMode::NavMeshGeneration)
    return;

  if (m_hCollisionMesh.IsValid())
  {
    ezResourceLock<ezPxMeshResource> pMesh(m_hCollisionMesh, ezResourceAcquireMode::BlockTillLoaded);

    pMesh->ExtractGeometry(GetOwner()->GetGlobalTransform(), msg);
  }
}

EZ_STATICLINK_FILE(PhysXPlugin, PhysXPlugin_Shapes_Implementation_PxShapeConvexComponent);
