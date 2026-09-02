#include <RendererCore/RendererCorePCH.h>

#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Lights/ReflectionProbePlacementComponent.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_ENUM(ezReflectionProbePlacementMode, 1)
  EZ_ENUM_CONSTANT(ezReflectionProbePlacementMode::Indoors),
  EZ_ENUM_CONSTANT(ezReflectionProbePlacementMode::Outdoors),
  EZ_ENUM_CONSTANT(ezReflectionProbePlacementMode::Both),
EZ_END_STATIC_REFLECTED_ENUM;

EZ_BEGIN_COMPONENT_TYPE(ezReflectionProbePlacementComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Extents", GetExtents, SetExtents)->AddAttributes(new ezDefaultValueAttribute(ezVec3(50.0f)), new ezClampValueAttribute(ezVec3(1.0f), ezVariant())),
    EZ_MEMBER_PROPERTY("CellSize", m_fCellSize)->AddAttributes(new ezDefaultValueAttribute(0.5f), new ezClampValueAttribute(0.1f, 4.0f)),
    EZ_MEMBER_PROPERTY("ProbeHeight", m_fProbeHeight)->AddAttributes(new ezDefaultValueAttribute(1.7f), new ezClampValueAttribute(0.0f, 100.0f)),
    EZ_MEMBER_PROPERTY("MinAreaSize", m_fMinAreaSize)->AddAttributes(new ezDefaultValueAttribute(4.0f), new ezClampValueAttribute(0.0f, 1000.0f)),
    EZ_MEMBER_PROPERTY("LargeAreaRadius", m_fLargeAreaRadius)->AddAttributes(new ezDefaultValueAttribute(1.5f), new ezClampValueAttribute(0.0f, 20.0f)),
    EZ_MEMBER_PROPERTY("MergeDistance", m_fMergeDistance)->AddAttributes(new ezDefaultValueAttribute(4.0f), new ezClampValueAttribute(0.0f, 100.0f)),
    EZ_MEMBER_PROPERTY("MaxProbes", m_uiMaxProbes)->AddAttributes(new ezDefaultValueAttribute(64), new ezClampValueAttribute(1, 1000)),
    EZ_MEMBER_PROPERTY("MaxProbeSize", m_fMaxProbeSize)->AddAttributes(new ezDefaultValueAttribute(12.0f), new ezClampValueAttribute(1.0f, 200.0f)),
    EZ_ENUM_MEMBER_PROPERTY("Mode", ezReflectionProbePlacementMode, m_Mode),
    EZ_MEMBER_PROPERTY("OutdoorSpacing", m_fOutdoorSpacing)->AddAttributes(new ezDefaultValueAttribute(30.0f), new ezClampValueAttribute(2.0f, 500.0f)),
    EZ_MEMBER_PROPERTY("MaxCeilingHeight", m_fMaxCeilingHeight)->AddAttributes(new ezDefaultValueAttribute(8.0f), new ezClampValueAttribute(1.0f, 100.0f)),
    EZ_MEMBER_PROPERTY("GenerateBoxProbes", m_bGenerateBoxProbes)->AddAttributes(new ezDefaultValueAttribute(true)),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgUpdateLocalBounds, OnUpdateLocalBounds),
  }
  EZ_END_MESSAGEHANDLERS;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Lighting/Reflections"),
    new ezLongOpAttribute("ezLongOpProxy_PlaceReflectionProbes"),
    new ezBoxManipulatorAttribute("Extents", 1.0f, true),
    new ezBoxVisualizerAttribute("Extents", 1.0f, ezColor::CornflowerBlue),
    new ezInDevelopmentAttribute(ezInDevelopmentAttribute::Phase::Beta),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE
// clang-format on

ezReflectionProbePlacementComponent::ezReflectionProbePlacementComponent() = default;
ezReflectionProbePlacementComponent::~ezReflectionProbePlacementComponent() = default;

void ezReflectionProbePlacementComponent::OnActivated()
{
  GetOwner()->UpdateLocalBounds();
}

void ezReflectionProbePlacementComponent::OnDeactivated()
{
  GetOwner()->UpdateLocalBounds();
}

void ezReflectionProbePlacementComponent::SetExtents(const ezVec3& vExtents)
{
  if (m_vExtents != vExtents)
  {
    m_vExtents = vExtents;

    if (IsActiveAndInitialized())
    {
      GetOwner()->UpdateLocalBounds();
    }
  }
}

void ezReflectionProbePlacementComponent::OnUpdateLocalBounds(ezMsgUpdateLocalBounds& ref_msg) const
{
  ref_msg.AddBounds(ezBoundingBoxSphere::MakeFromBox(ezBoundingBox::MakeFromMinMax(-m_vExtents * 0.5f, m_vExtents * 0.5f)), ezInvalidSpatialDataCategory);
}

void ezReflectionProbePlacementComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  ezStreamWriter& s = inout_stream.GetStream();

  s << m_vExtents;
  s << m_fCellSize;
  s << m_fProbeHeight;
  s << m_fMinAreaSize;
  s << m_fLargeAreaRadius;
  s << m_fMergeDistance;
  s << m_uiMaxProbes;
  s << m_fMaxProbeSize;
  s << m_fMaxCeilingHeight;
  s << m_fOutdoorSpacing;
  s << m_Mode;
  s << m_bGenerateBoxProbes;
}

void ezReflectionProbePlacementComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);

  ezStreamReader& s = inout_stream.GetStream();

  s >> m_vExtents;
  s >> m_fCellSize;
  s >> m_fProbeHeight;
  s >> m_fMinAreaSize;
  s >> m_fLargeAreaRadius;
  s >> m_fMergeDistance;
  s >> m_uiMaxProbes;
  s >> m_fMaxProbeSize;
  s >> m_fMaxCeilingHeight;
  s >> m_fOutdoorSpacing;
  s >> m_Mode;
  s >> m_bGenerateBoxProbes;
}

EZ_STATICLINK_FILE(RendererCore, RendererCore_Lights_Implementation_ReflectionProbePlacementComponent);
