#include <RendererCore/RendererCorePCH.h>

#include <Core/Messages/HierarchyChangedMessages.h>
#include <Core/Messages/TransformChangedMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Components/PathComponent.h>
#include <RendererCore/Debug/DebugRenderer.h>

// clang-format off
EZ_IMPLEMENT_MESSAGE_TYPE(ezEventMsgPathChanged);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezEventMsgPathChanged, 1, ezRTTIDefaultAllocator<ezEventMsgPathChanged>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

static const ezTypeVersion s_PathNodeVersion = 1;

ezResult ezPathNodeData::Serialize(ezStreamWriter& writer) const
{
  writer.WriteVersion(s_PathNodeVersion);

  writer << m_vPosition;
  writer << m_qRotation;
  writer << m_Color;

  return EZ_SUCCESS;
}

ezResult ezPathNodeData::Deserialize(ezStreamReader& reader)
{
  /*auto version = */ reader.ReadVersion(s_PathNodeVersion);

  reader >> m_vPosition;
  reader >> m_qRotation;
  reader >> m_Color;

  return EZ_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezPathComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezEventMsgPathChanged, OnEventMsgPathChanged),
  }
  EZ_END_MESSAGEHANDLERS;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Animation"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE
// clang-format on

ezPathComponent::ezPathComponent() = default;
ezPathComponent::~ezPathComponent() = default;

void ezPathComponent::Update()
{
  if (m_bPathChanged)
  {
    UpdatePath();
  }

  if (m_PathNodes.IsEmpty())
    return;

  ezHybridArray<ezDebugRenderer::Line, 32> lines;

  ezUInt32 uiPrev = m_bLooping ? m_PathNodes.GetCount() - 1 : 0;
  ezUInt32 uiNext = m_bLooping ? 0 : 1;

  for (; uiNext < m_PathNodes.GetCount(); ++uiNext)
  {
    const auto& n0 = m_PathNodes[uiPrev];
    const auto& n1 = m_PathNodes[uiNext];

    auto& line = lines.ExpandAndGetRef();
    line.m_start = n0.m_vPosition;
    line.m_startColor = n0.m_Color;
    line.m_end = n1.m_vPosition;
    line.m_endColor = n1.m_Color;

    uiPrev = uiNext;
  }

  ezDebugRenderer::DrawLines(GetWorld(), lines, ezColor::White); // GetOwner()->GetGlobalTransform());
}

void ezPathComponent::OnEventMsgPathChanged(ezEventMsgPathChanged& msg)
{
  m_bPathChanged = true;
}

void ezPathComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  auto& s = stream.GetStream();
  stream.GetStream().WriteArray(m_PathNodes).AssertSuccess();

  s << m_bLooping;
}

void ezPathComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);

  auto& s = stream.GetStream();
  stream.GetStream().ReadArray(m_PathNodes).AssertSuccess();

  s >> m_bLooping;

  m_bPathChanged = false;
}

void ezPathComponent::UpdatePath()
{
  m_PathNodes.Clear();
  m_bLooping = false;
  m_bPathChanged = false;

  auto resolver = GetWorld()->GetGameObjectReferenceResolver();
  if (!resolver.IsValid())
    return;

  struct Targets
  {
    bool m_bVisited = false;
    ezGameObjectHandle m_hT1;
    ezGameObjectHandle m_hT2;
  };

  ezMap<ezGameObjectHandle, Targets> connections;

  auto HookUp = [&](ezGameObjectHandle src, ezGameObjectHandle dst) {
    auto& data = connections[src];

    if (data.m_hT1 == dst || data.m_hT2 == dst)
      return;

    if (data.m_hT1.IsInvalidated())
    {
      data.m_hT1 = dst;
      return;
    }

    if (data.m_hT2.IsInvalidated())
    {
      data.m_hT2 = dst;
      return;
    }

    // else ignore connection
  };

  for (auto it = GetOwner()->GetChildren(); it.IsValid(); it.Next())
  {
    ezPathNodeComponent* pNode = nullptr;
    if (!it->TryGetComponentOfBaseType(pNode))
      continue;

    for (const auto& con : pNode->m_Connections)
    {
      ezGameObjectHandle hTarget = resolver(con.m_sTarget, GetHandle(), nullptr);

      if (!hTarget.IsInvalidated())
      {
        HookUp(it->GetHandle(), hTarget);
        HookUp(hTarget, it->GetHandle());
      }
    }
  }

  if (connections.IsEmpty())
    return;

  ezGameObjectHandle hCurNode = connections.GetIterator().Key();

  for (auto it = connections.GetIterator(); it.IsValid(); ++it)
  {
    if (it.Value().m_hT2.IsInvalidated())
    {
      hCurNode = it.Key();
      break;
    }
  }

  ezGameObjectHandle hPrevNode;

  ezWorld* pWorld = GetWorld();

  while (true)
  {
    auto& con = connections[hCurNode];
    if (con.m_bVisited)
    {
      m_bLooping = true;
      break;
    }

    con.m_bVisited = true;

    ezGameObject* pCurObj;
    if (pWorld->TryGetObject(hCurNode, pCurObj))
    {
      auto& nd = m_PathNodes.ExpandAndGetRef();
      nd.m_vPosition = pCurObj->GetGlobalTransform().m_vPosition;
      nd.m_Color = ezColor::White;
    }


    ezGameObjectHandle hNextNode;
    if (con.m_hT1 != hPrevNode)
      hNextNode = con.m_hT1;
    else
      hNextNode = con.m_hT2;

    if (hNextNode.IsInvalidated())
      break;

    hPrevNode = hCurNode;
    hCurNode = hNextNode;
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_TYPE(ezPathNodeConnection, ezNoBase, 1, ezRTTIDefaultAllocator<ezPathNodeConnection>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Target", m_sTarget)->AddAttributes(new ezGameObjectReferenceAttribute()),
  }
  EZ_END_PROPERTIES;
}
EZ_END_STATIC_REFLECTED_TYPE
// clang-format on

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezPathNodeComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ARRAY_ACCESSOR_PROPERTY("Connections", Connections_GetCount, Connections_GetValue, Connections_SetValue, Connections_Insert, Connections_Remove),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgTransformChanged, OnMsgTransformChanged),
    EZ_MESSAGE_HANDLER(ezMsgParentChanged, OnMsgParentChanged),
  }
  EZ_END_MESSAGEHANDLERS;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Animation"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE
// clang-format on

ezPathNodeComponent::ezPathNodeComponent() = default;
ezPathNodeComponent::~ezPathNodeComponent() = default;

void ezPathNodeComponent::OnMsgTransformChanged(ezMsgTransformChanged& msg)
{
  PathChanged();
}

void ezPathNodeComponent::OnMsgParentChanged(ezMsgParentChanged& msg)
{
  PathChanged();
}

void ezPathNodeComponent::OnActivated()
{
  SUPER::OnActivated();

  GetOwner()->EnableStaticTransformChangesNotifications();
  GetOwner()->EnableParentChangesNotifications();

  PathChanged();
}

ezUInt32 ezPathNodeComponent::Connections_GetCount() const
{
  return m_Connections.GetCount();
}

const ezPathNodeConnection& ezPathNodeComponent::Connections_GetValue(ezUInt32 uiIndex) const
{
  return m_Connections[uiIndex];
}

void ezPathNodeComponent::Connections_SetValue(ezUInt32 uiIndex, const ezPathNodeConnection& value)
{
  m_Connections[uiIndex] = value;
  PathChanged();
}

void ezPathNodeComponent::Connections_Insert(ezUInt32 uiIndex, const ezPathNodeConnection& value)
{
  m_Connections.Insert(value, uiIndex);
  PathChanged();
}

void ezPathNodeComponent::Connections_Remove(ezUInt32 uiIndex)
{
  m_Connections.RemoveAtAndCopy(uiIndex);
  PathChanged();
}

void ezPathNodeComponent::PathChanged()
{
  if (!IsActiveAndInitialized())
    return;

  ezEventMsgPathChanged msg2;
  // msg2.SetDebugMessageRouting(true);
  GetOwner()->SendEventMessage(msg2, this);
}
