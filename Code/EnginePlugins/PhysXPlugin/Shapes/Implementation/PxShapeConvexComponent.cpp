#include <PhysXPlugin/PhysXPluginPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <PhysXPlugin/Shapes/PxShapeConvexComponent.h>
#include <PhysXPlugin/Utilities/PxConversionUtils.h>
#include <PhysXPlugin/WorldModule/Implementation/PhysX.h>
#include <RendererCore/Utils/WorldGeoExtractionUtil.h>
#include <extensions/PxRigidActorExt.h>

using namespace physx;

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezPxShapeConvexComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("CollisionMesh", GetMeshFile, SetMeshFile)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_PhysX_Colmesh_Convex")),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezPxShapeConvexComponent::ezPxShapeConvexComponent() = default;
ezPxShapeConvexComponent::~ezPxShapeConvexComponent() = default;

void ezPxShapeConvexComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();

  s << m_hCollisionMesh;
}

void ezPxShapeConvexComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const ezUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = inout_stream.GetStream();

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

void ezPxShapeConvexComponent::CreateShapes(ezDynamicArray<physx::PxShape*>& out_Shapes, physx::PxRigidActor* pActor, physx::PxTransform& out_ShapeTransform)
{
  if (!m_hCollisionMesh.IsValid())
  {
    ezLog::Warning("ezPxShapeConvexComponent '{0}' has no collision mesh set.", GetOwner()->GetName());
    return;
  }

  ezResourceLock<ezPxMeshResource> pMesh(m_hCollisionMesh, ezResourceAcquireMode::BlockTillLoaded);

  if (pMesh->GetConvexParts().IsEmpty())
  {
    ezLog::Warning("ezPxShapeConvexComponent '{0}' has a collision mesh set that does not contain a convex mesh: '{1}' ('{2}')", GetOwner()->GetName(), pMesh->GetResourceID(), pMesh->GetResourceDescription());
    return;
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
      ezResourceLock<ezSurfaceResource> pSurface(surfaces[0], ezResourceAcquireMode::BlockTillLoaded);
      pMaterial = static_cast<PxMaterial*>(pSurface->m_pPhysicsMaterialPhysX);
    }
  }

  if (pMaterial == nullptr)
  {
    pMaterial = ezPhysX::GetSingleton()->GetDefaultMaterial();
  }

  PxMeshScale scale = ezPxConversionUtils::ToScale(GetOwner()->GetGlobalTransformSimd());

  for (auto pShape : pMesh->GetConvexParts())
  {
    out_Shapes.PushBack(PxRigidActorExt::createExclusiveShape(*pActor, PxConvexMeshGeometry(pShape, scale), *pMaterial));
  }
}

void ezPxShapeConvexComponent::ExtractGeometry(ezMsgExtractGeometry& ref_msg) const
{
  if (ref_msg.m_Mode != ezWorldGeoExtractionUtil::ExtractionMode::CollisionMesh && ref_msg.m_Mode != ezWorldGeoExtractionUtil::ExtractionMode::NavMeshGeneration)
    return;

  if (m_hCollisionMesh.IsValid())
  {
    ezResourceLock<ezPxMeshResource> pMesh(m_hCollisionMesh, ezResourceAcquireMode::BlockTillLoaded);

    ref_msg.AddMeshObject(GetOwner()->GetGlobalTransform(), pMesh->ConvertToCpuMesh());
  }
}

EZ_STATICLINK_FILE(PhysXPlugin, PhysXPlugin_Shapes_Implementation_PxShapeConvexComponent);
