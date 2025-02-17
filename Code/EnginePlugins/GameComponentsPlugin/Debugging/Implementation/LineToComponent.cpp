#include <GameComponentsPlugin/GameComponentsPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameComponentsPlugin/Debugging/LineToComponent.h>
#include <RendererCore/Debug/DebugRenderer.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezLineToComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    // BEGIN-DOCS-CODE-SNIPPET: object-reference-property
    EZ_ACCESSOR_PROPERTY("Target", GetLineToTargetGuid, SetLineToTargetGuid)->AddAttributes(new ezGameObjectReferenceAttribute()),
    // END-DOCS-CODE-SNIPPET
    EZ_MEMBER_PROPERTY("Color", m_LineColor)->AddAttributes(new ezDefaultValueAttribute(ezColor::Orange)),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Utilities/Debug"),
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

  ezDynamicArray<ezDebugRendererLine> lines;

  auto& line = lines.ExpandAndGetRef();
  line.m_start = GetOwner()->GetGlobalPosition();
  line.m_end = pTarget->GetGlobalPosition();

  ezDebugRenderer::DrawLines(GetWorld(), lines, m_LineColor);
}

void ezLineToComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  inout_stream.WriteGameObjectHandle(m_hTargetObject);
  s << m_LineColor;
}

void ezLineToComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = inout_stream.GetStream();

  m_hTargetObject = inout_stream.ReadGameObjectHandle();
  s >> m_LineColor;
}

void ezLineToComponent::SetLineToTarget(const ezGameObjectHandle& hTargetObject)
{
  m_hTargetObject = hTargetObject;
}

// BEGIN-DOCS-CODE-SNIPPET: object-reference-funcs
void ezLineToComponent::SetLineToTargetGuid(const char* szTargetGuid)
{
  auto resolver = GetWorld()->GetGameObjectReferenceResolver();

  if (resolver.IsValid())
  {
    // tell the resolver our component handle and the name of the property for the object reference
    m_hTargetObject = resolver(szTargetGuid, GetHandle(), "Target");
  }
}

const char* ezLineToComponent::GetLineToTargetGuid() const
{
  // this function is never called
  return nullptr;
}
// END-DOCS-CODE-SNIPPET


EZ_STATICLINK_FILE(GameComponentsPlugin, GameComponentsPlugin_Debugging_Implementation_LineToComponent);
