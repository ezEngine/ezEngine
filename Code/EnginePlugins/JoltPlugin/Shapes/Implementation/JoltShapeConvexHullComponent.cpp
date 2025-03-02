#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/Messages/UpdateLocalBoundsMessage.h>
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
    EZ_RESOURCE_MEMBER_PROPERTY("CollisionMesh", m_hCollisionMesh)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Jolt_Colmesh_Convex", ezDependencyFlags::Package)),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgUpdateLocalBounds, OnUpdateLocalBounds),
  }
  EZ_END_MESSAGEHANDLERS;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezJoltShapeConvexHullComponent::ezJoltShapeConvexHullComponent() = default;
ezJoltShapeConvexHullComponent::~ezJoltShapeConvexHullComponent() = default;

void ezJoltShapeConvexHullComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();

  s << m_hCollisionMesh;
}

void ezJoltShapeConvexHullComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const ezUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = inout_stream.GetStream();

  s >> m_hCollisionMesh;
}

void ezJoltShapeConvexHullComponent::OnUpdateLocalBounds(ezMsgUpdateLocalBounds& msg) const
{
  if (m_hCollisionMesh.IsValid())
  {
    ezResourceLock<ezJoltMeshResource> pMesh(m_hCollisionMesh, ezResourceAcquireMode::BlockTillLoaded);
    msg.AddBounds(pMesh->GetBounds(), ezInvalidSpatialDataCategory);
  }
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
    ezLog::Warning("ezJoltShapeConvexHullComponent '{0}' has a collision mesh set that does not contain a convex mesh: '{1}'", GetOwner()->GetName(), pMesh->GetResourceIdOrDescription());
    return;
  }

  for (ezUInt32 i = 0; i < pMesh->GetNumConvexParts(); ++i)
  {
    auto pShape = pMesh->InstantiateConvexPart(i, reinterpret_cast<ezUInt64>(GetUserData()), pMaterial, fDensity);

    ezJoltSubShape& sub = out_Shapes.ExpandAndGetRef();
    sub.m_pShape = pShape;
    sub.m_Transform = ezTransform::MakeLocalTransform(rootTransform, GetOwner()->GetGlobalTransform());
  }
}

void ezJoltShapeConvexHullComponent::ExtractGeometry(ezMsgExtractGeometry& ref_msg) const
{
  if (ref_msg.m_Mode != ezWorldGeoExtractionUtil::ExtractionMode::CollisionMesh)
    return;

  if (m_hCollisionMesh.IsValid())
  {
    ezResourceLock<ezJoltMeshResource> pMesh(m_hCollisionMesh, ezResourceAcquireMode::BlockTillLoaded);

    ref_msg.AddMeshObject(GetOwner()->GetGlobalTransform(), pMesh->ConvertToCpuMesh());
  }
}


EZ_STATICLINK_FILE(JoltPlugin, JoltPlugin_Shapes_Implementation_JoltShapeConvexHullComponent);
