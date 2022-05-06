#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/Physics/SurfaceResource.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Jolt/Physics/Collision/Shape/ConvexHullShape.h>
#include <JoltPlugin/Resources/JoltMaterial.h>
#include <JoltPlugin/Resources/JoltMeshResource.h>
#include <JoltPlugin/Shapes/JoltShapeConvexHullComponent.h>
#include <JoltPlugin/Utilities/JoltConversionUtils.h>
#include <RendererCore/Utils/WorldGeoExtractionUtil.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezJoltShapeConvexHullComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("CollisionMesh", GetMeshFile, SetMeshFile)->AddAttributes(new ezAssetBrowserAttribute("Jolt_Colmesh_Convex")),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezJoltShapeConvexHullComponent::ezJoltShapeConvexHullComponent() = default;
ezJoltShapeConvexHullComponent::~ezJoltShapeConvexHullComponent() = default;

void ezJoltShapeConvexHullComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  auto& s = stream.GetStream();

  s << m_hCollisionMesh;
}

void ezJoltShapeConvexHullComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = stream.GetStream();

  s >> m_hCollisionMesh;
}

void ezJoltShapeConvexHullComponent::SetMeshFile(const char* szFile)
{
  if (!ezStringUtils::IsNullOrEmpty(szFile))
  {
    m_hCollisionMesh = ezResourceManager::LoadResource<ezJoltMeshResource>(szFile);
  }
}

const char* ezJoltShapeConvexHullComponent::GetMeshFile() const
{
  if (!m_hCollisionMesh.IsValid())
    return "";

  return m_hCollisionMesh.GetResourceID();
}

void ezJoltShapeConvexHullComponent::CreateShapes(ezDynamicArray<ezJoltSubShape>& out_Shapes, const ezTransform& rootTransform, float fDensity, const ezJoltMaterial* pMaterial)
{
  if (!m_hCollisionMesh.IsValid())
  {
    ezLog::Warning("ezJoltShapeConvexHullComponent '{0}' has no collision mesh set.", GetOwner()->GetName());
    return;
  }

  ezResourceLock<ezJoltMeshResource> pMesh(m_hCollisionMesh, ezResourceAcquireMode::BlockTillLoaded);

  if (pMesh->GetNumConvexParts() == 0)
  {
    ezLog::Warning("ezJoltShapeConvexHullComponent '{0}' has a collision mesh set that does not contain a convex mesh: '{1}' ('{2}')", GetOwner()->GetName(), pMesh->GetResourceID(), pMesh->GetResourceDescription());
    return;
  }

  for (ezUInt32 i = 0; i < pMesh->GetNumConvexParts(); ++i)
  {
    auto pShape = pMesh->InstantiateConvexPart(i, reinterpret_cast<ezUInt64>(GetUserData()), pMaterial, fDensity);

    ezJoltSubShape& sub = out_Shapes.ExpandAndGetRef();
    sub.m_pShape = pShape;
    sub.m_Transform.SetLocalTransform(rootTransform, GetOwner()->GetGlobalTransform());
  }
}

void ezJoltShapeConvexHullComponent::ExtractGeometry(ezMsgExtractGeometry& msg) const
{
  if (msg.m_Mode != ezWorldGeoExtractionUtil::ExtractionMode::CollisionMesh && msg.m_Mode != ezWorldGeoExtractionUtil::ExtractionMode::NavMeshGeneration)
    return;

  if (m_hCollisionMesh.IsValid())
  {
    ezResourceLock<ezJoltMeshResource> pMesh(m_hCollisionMesh, ezResourceAcquireMode::BlockTillLoaded);

    msg.AddMeshObject(GetOwner()->GetGlobalTransform(), pMesh->ConvertToCpuMesh());
  }
}
