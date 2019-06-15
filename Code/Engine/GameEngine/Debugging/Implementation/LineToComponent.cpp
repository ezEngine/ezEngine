#include <GameEnginePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Debugging/LineToComponent.h>
#include <RendererCore/Debug/DebugRenderer.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezLineToComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Target", GetLineToTargetGuid, SetLineToTargetGuid),
    EZ_MEMBER_PROPERTY("Color", m_LineColor)->AddAttributes(new ezDefaultValueAttribute(ezColor::Orange)),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Debug"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezLineToComponent::ezLineToComponent() = default;
ezLineToComponent::~ezLineToComponent() = default;

void ezLineToComponent::Update()
{
  if (m_hTargetObject.IsInvalidated())
    return;

  ezGameObject* pTarget = nullptr;
  if (!GetWorld()->TryGetObject(m_hTargetObject, pTarget))
  {
    m_hTargetObject.Invalidate();
    return;
  }

  ezDynamicArray<ezDebugRenderer::Line> lines;

  auto& line = lines.ExpandAndGetRef();
  line.m_start = GetOwner()->GetGlobalPosition();
  line.m_end = pTarget->GetGlobalPosition();

  ezDebugRenderer::DrawLines(GetWorld(), lines, m_LineColor);
}

void ezLineToComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  auto& s = stream.GetStream();

  stream.WriteGameObjectHandle(m_hTargetObject);
  s << m_LineColor;
}

void ezLineToComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  // const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = stream.GetStream();

  m_hTargetObject = stream.ReadGameObjectHandle();
  s >> m_LineColor;
}

void ezLineToComponent::SetLineToTarget(const ezGameObjectHandle& hTargetObject)
{
  m_hTargetObject = hTargetObject;
}

void ezLineToComponent::SetLineToTargetGuid(const char* szTargetGuid)
{
  auto resolver = GetWorld()->GetGameObjectReferenceResolver();

  if (!resolver.IsValid())
    return;

  m_hTargetObject = resolver(szTargetGuid);
}

const char* ezLineToComponent::GetLineToTargetGuid() const
{
  return "REMEMBER ME!";
}
