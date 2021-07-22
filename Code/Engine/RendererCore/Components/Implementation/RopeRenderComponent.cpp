#include <RendererCorePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/AnimationSystem/Declarations.h>
#include <RendererCore/Components/RopeRenderComponent.h>
#include <RendererCore/Debug/DebugRenderer.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezRopeRenderComponent, 2, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Color", m_Color)->AddAttributes(new ezDefaultValueAttribute(ezColor::Black))
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgRopePoseUpdated, OnRopePoseUpdated),
  }
  EZ_END_MESSAGEHANDLERS;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Effects"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezRopeRenderComponent::ezRopeRenderComponent() = default;
ezRopeRenderComponent::~ezRopeRenderComponent() = default;

ezResult ezRopeRenderComponent::GetLocalBounds(ezBoundingBoxSphere& bounds, bool& bAlwaysVisible)
{
  bounds = m_LocalBounds;
  return EZ_SUCCESS;
}

void ezRopeRenderComponent::Update()
{
  ezDebugRenderer::DrawLines(GetWorld(), m_Lines, ezColor::White, GetOwner()->GetGlobalTransform());
}

void ezRopeRenderComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  auto& s = stream.GetStream();

  s << m_Color;
}

void ezRopeRenderComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = stream.GetStream();

  if (uiVersion >= 2)
  {
    s >> m_Color;
  }
}

void ezRopeRenderComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  // TODO: generate meshes etc
}

void ezRopeRenderComponent::OnRopePoseUpdated(ezMsgRopePoseUpdated& msg)
{
  m_Lines.Clear();

  ezBoundingSphere bsphere;
  bsphere.SetInvalid();
  bsphere.m_fRadius = 0.0f;

  if (!msg.m_LinkTransforms.IsEmpty())
  {
    const ezUInt32 uiCenterLink = msg.m_LinkTransforms.GetCount() / 2;
    bsphere.m_vCenter = msg.m_LinkTransforms[uiCenterLink].m_vPosition;

    m_Lines.Reserve(m_Lines.GetCount() + msg.m_LinkTransforms.GetCount());

    bsphere.ExpandToInclude(msg.m_LinkTransforms[0].m_vPosition);

    for (ezUInt32 i = 1; i < msg.m_LinkTransforms.GetCount(); ++i)
    {
      auto& l = m_Lines.ExpandAndGetRef();
      l.m_start = msg.m_LinkTransforms[i - 1].m_vPosition;
      l.m_end = msg.m_LinkTransforms[i].m_vPosition;
      l.m_startColor = ezColor::Black;
      l.m_endColor = ezColor::Black;

      bsphere.ExpandToInclude(l.m_end);
    }
  }

  // if the existing bounds are big enough, don't update them
  if (!m_LocalBounds.IsValid() || !m_LocalBounds.GetSphere().Contains(bsphere))
  {
    m_LocalBounds.SetInvalid();
    m_LocalBounds.ExpandToInclude(bsphere);

    TriggerLocalBoundsUpdate();
  }
}
