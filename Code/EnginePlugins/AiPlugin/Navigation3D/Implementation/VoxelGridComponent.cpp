#include <AiPlugin/AiPluginPCH.h>

#include <AiPlugin/Navigation3D/VoxelGridComponent.h>
#include <AiPlugin/Navigation3D/VoxelWorldModule.h>
#include <Core/Interfaces/NavmeshGeoWorldModule.h>
#include <Core/World/World.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezAiVoxelGridComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Size", GetSize, SetSize)->AddAttributes(new ezDefaultValueAttribute(ezVec3(32)), new ezClampValueAttribute(ezVec3(4), ezVec3(1024))),
    EZ_ACCESSOR_PROPERTY("VoxelSize", GetVoxelSize, SetVoxelSize)->AddAttributes(new ezDefaultValueAttribute(0.5f), new ezClampValueAttribute(0.1f, 10.0f)),
    EZ_ACCESSOR_PROPERTY("CollisionLayer", GetCollisionLayer, SetCollisionLayer)->AddAttributes(new ezDynamicEnumAttribute("PhysicsCollisionLayer")),
    EZ_MEMBER_PROPERTY("Visualize", m_bVisualize),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgUpdateLocalBounds, OnMsgUpdateLocalBounds)
  }
  EZ_END_MESSAGEHANDLERS;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("AI/Navigation"),
    new ezBoxVisualizerAttribute("Size", 1.0f, ezColorScheme::LightUI(ezColorScheme::Green), nullptr),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezSpatialData::Category ezAiVoxelGridComponent::SpatialDataCategory = ezSpatialData::RegisterCategory("AiVoxelGrid", ezSpatialData::Flags::None);

ezAiVoxelGridComponent::ezAiVoxelGridComponent() = default;
ezAiVoxelGridComponent::~ezAiVoxelGridComponent() = default;

void ezAiVoxelGridComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_vSize;
  s << m_fVoxelSize;
  s << m_uiCollisionLayer;
  s << m_bVisualize;
}

void ezAiVoxelGridComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const ezUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_vSize;
  s >> m_fVoxelSize;
  s >> m_uiCollisionLayer;
  s >> m_bVisualize;
}

void ezAiVoxelGridComponent::OnActivated()
{
  SUPER::OnActivated();

  GetOwner()->UpdateLocalBounds();
}

void ezAiVoxelGridComponent::OnDeactivated()
{
  GetOwner()->UpdateLocalBounds();

  SUPER::OnDeactivated();
}

void ezAiVoxelGridComponent::OnMsgUpdateLocalBounds(ezMsgUpdateLocalBounds& msg) const
{
  msg.AddBounds(ezBoundingBoxSphere::MakeFromBox(ezBoundingBox::MakeFromCenterAndHalfExtents(ezVec3::MakeZero(), m_vSize * 0.5f)), SpatialDataCategory);
}

void ezAiVoxelGridComponent::SetSize(const ezVec3& vSize)
{
  if (m_vSize != vSize)
  {
    m_vSize = vSize;
    m_bNeedsVoxelization = true;

    if (IsActiveAndInitialized())
    {
      GetOwner()->UpdateLocalBounds();
    }
  }
}

void ezAiVoxelGridComponent::SetVoxelSize(float fValue)
{
  if (m_fVoxelSize != fValue)
  {
    m_fVoxelSize = fValue;
    m_bNeedsVoxelization = true;
  }
}

void ezAiVoxelGridComponent::SetCollisionLayer(ezUInt32 uiValue)
{
  if (m_uiCollisionLayer != uiValue)
  {
    m_uiCollisionLayer = uiValue;
    m_bNeedsVoxelization = true;
  }
}

void ezAiVoxelGridComponent::VoxelizeWorld()
{
  if (!m_bNeedsVoxelization)
    return;

  // retrieve collision geometry and rasterize triangles
  auto* pGeoModule = GetWorld()->GetOrCreateModule<ezNavmeshGeoWorldModuleInterface>();
  if (pGeoModule == nullptr)
    return;

  m_bNeedsVoxelization = false;

  ezVec3U32 res;
  res.x = ezMath::CeilToInt(m_vSize.x / m_fVoxelSize);
  res.y = ezMath::CeilToInt(m_vSize.y / m_fVoxelSize);
  res.z = ezMath::CeilToInt(m_vSize.z / m_fVoxelSize);

  ezVec3 vGridCenter = GetOwner()->GetGlobalPosition();

  m_StaticGrid.Initialize(res, vGridCenter, m_fVoxelSize);

  const ezBoundingBox gridAABB = m_StaticGrid.GetAABB();

  ezDynamicArray<ezNavmeshTriangle> triangles;
  pGeoModule->RetrieveGeometryInArea(m_uiCollisionLayer, gridAABB, triangles);

  for (const auto& tri : triangles)
  {
    m_StaticGrid.SetVoxelsOnTriangle(tri.m_Vertices[0], tri.m_Vertices[1], tri.m_Vertices[2]);
  }
}


EZ_STATICLINK_FILE(AiPlugin, AiPlugin_Navigation3D_Implementation_VoxelGridComponent);
