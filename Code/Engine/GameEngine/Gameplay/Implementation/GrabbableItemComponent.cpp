#include <GameEngine/GameEnginePCH.h>

#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Gameplay/GrabbableItemComponent.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Pipeline/View.h>

struct GICFlags
{
  enum Enum
  {
    DebugShowPoints = 0,
  };
};

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_TYPE(ezGrabbableItemGrabPoint, ezNoBase, 1, ezRTTIDefaultAllocator<ezGrabbableItemGrabPoint>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("LocalPosition", m_vLocalPosition),
    EZ_MEMBER_PROPERTY("LocalRotation", m_qLocalRotation),
  }
  EZ_END_PROPERTIES;

  EZ_BEGIN_ATTRIBUTES
  {
    new ezTransformManipulatorAttribute("LocalPosition", "LocalRotation"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_STATIC_REFLECTED_TYPE


EZ_BEGIN_COMPONENT_TYPE(ezGrabbableItemComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("DebugShowPoints", GetDebugShowPoints, SetDebugShowPoints),
    EZ_ARRAY_MEMBER_PROPERTY("GrabPoints", m_GrabPoints),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgUpdateLocalBounds, OnUpdateLocalBounds),
    EZ_MESSAGE_HANDLER(ezMsgExtractRenderData, OnExtractRenderData),
  }
  EZ_END_MESSAGEHANDLERS;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Input"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezGrabbableItemComponent::ezGrabbableItemComponent() = default;
ezGrabbableItemComponent::~ezGrabbableItemComponent() = default;

void ezGrabbableItemComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  const ezUInt8 uiNumGrabPoints = static_cast<ezUInt8>(m_GrabPoints.GetCount());
  s << uiNumGrabPoints;
  for (const auto& gb : m_GrabPoints)
  {
    s << gb.m_vLocalPosition;
    s << gb.m_qLocalRotation;
  }
}

void ezGrabbableItemComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  ezUInt8 uiNumGrabPoints;
  s >> uiNumGrabPoints;
  m_GrabPoints.SetCount(uiNumGrabPoints);
  for (auto& gb : m_GrabPoints)
  {
    s >> gb.m_vLocalPosition;
    s >> gb.m_qLocalRotation;
  }
}

void ezGrabbableItemComponent::SetDebugShowPoints(bool bShow)
{
  SetUserFlag(GICFlags::DebugShowPoints, bShow);

  if (IsActiveAndInitialized())
  {
    GetOwner()->UpdateLocalBounds();
  }
}

bool ezGrabbableItemComponent::GetDebugShowPoints() const
{
  return GetUserFlag(GICFlags::DebugShowPoints);
}

void ezGrabbableItemComponent::OnUpdateLocalBounds(ezMsgUpdateLocalBounds& msg) const
{
  if (GetDebugShowPoints())
  {
    msg.AddBounds(ezBoundingSphere::MakeFromCenterAndRadius(ezVec3::MakeZero(), 1.0f), ezDefaultSpatialDataCategories::RenderDynamic);
  }
}

void ezGrabbableItemComponent::OnExtractRenderData(ezMsgExtractRenderData& msg) const
{
  if (!GetDebugShowPoints() || m_GrabPoints.IsEmpty())
    return;

  if (msg.m_pView->GetCameraUsageHint() != ezCameraUsageHint::MainView &&
      msg.m_pView->GetCameraUsageHint() != ezCameraUsageHint::EditorView)
    return;

  // Don't extract render data for selection.
  if (msg.m_OverrideCategory != ezInvalidRenderDataCategory)
    return;

  const ezTransform globalTransform = GetOwner()->GetGlobalTransform();

  for (auto& grabPoint : m_GrabPoints)
  {
    ezTransform grabPointTransform = ezTransform::MakeGlobalTransform(globalTransform, ezTransform(grabPoint.m_vLocalPosition, grabPoint.m_qLocalRotation));

    ezDebugRenderer::DrawArrow(GetWorld(), 0.75f, ezColorScheme::LightUI(ezColorScheme::Red), grabPointTransform, ezVec3::MakeAxisX());
    ezDebugRenderer::DrawArrow(GetWorld(), 0.3f, ezColorScheme::LightUI(ezColorScheme::Green), grabPointTransform, ezVec3::MakeAxisY());
    ezDebugRenderer::DrawArrow(GetWorld(), 0.3f, ezColorScheme::LightUI(ezColorScheme::Blue), grabPointTransform, ezVec3::MakeAxisZ());
  }
}


EZ_STATICLINK_FILE(GameEngine, GameEngine_Gameplay_Implementation_GrabbableItemComponent);
