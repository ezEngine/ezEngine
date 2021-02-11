#include <GameEnginePCH.h>

#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Gameplay/BlackboardComponent.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Pipeline/View.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_TYPE(ezBlackboardEntry, ezNoBase, 1, ezRTTIDefaultAllocator<ezBlackboardEntry>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Name", GetName, SetName),
    EZ_MEMBER_PROPERTY("InitialValue", m_InitialValue)->AddAttributes(new ezDefaultValueAttribute(0)),
    EZ_BITFLAGS_MEMBER_PROPERTY("Flags", ezBlackboardEntryFlags, m_Flags)
  }
  EZ_END_PROPERTIES;
}
EZ_END_STATIC_REFLECTED_TYPE;
// clang-format on

ezResult ezBlackboardEntry::Serialize(ezStreamWriter& stream) const
{
  stream << m_sName;
  stream << m_InitialValue;
  stream << m_Flags;

  return EZ_SUCCESS;
}

ezResult ezBlackboardEntry::Deserialize(ezStreamReader& stream)
{
  stream >> m_sName;
  stream >> m_InitialValue;
  stream >> m_Flags;

  return EZ_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezBlackboardComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("ShowDebugInfo", GetShowDebugInfo, SetShowDebugInfo),
    EZ_ARRAY_ACCESSOR_PROPERTY("Entries", Entries_GetCount, Entries_GetValue, Entries_SetValue, Entries_Insert, Entries_Remove),
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
    new ezCategoryAttribute("Gameplay"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE
// clang-format on

ezBlackboardComponent::ezBlackboardComponent()
  : m_pBoard(EZ_DEFAULT_NEW(ezBlackboard))
{
}

ezBlackboardComponent::ezBlackboardComponent(ezBlackboardComponent&& other) = default;
ezBlackboardComponent::~ezBlackboardComponent() = default;
ezBlackboardComponent& ezBlackboardComponent::operator=(ezBlackboardComponent&& other) = default;

void ezBlackboardComponent::OnActivated()
{
  if (GetShowDebugInfo())
  {
    GetOwner()->UpdateLocalBounds();
  }
}

void ezBlackboardComponent::OnDeactivated()
{
  if (GetShowDebugInfo())
  {
    GetOwner()->UpdateLocalBounds();
  }
}

void ezBlackboardComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  ezStreamWriter& s = stream.GetStream();

  s.WriteArray(m_InitialEntries).IgnoreResult();
}

void ezBlackboardComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  ezStreamReader& s = stream.GetStream();

  ezDynamicArray<ezBlackboardEntry> initialEntries;
  if (s.ReadArray(initialEntries).Succeeded())
  {
    RegisterEntries(initialEntries);
  }
}

ezBlackboard& ezBlackboardComponent::GetBoard()
{
  return *m_pBoard.Borrow();
}

const ezBlackboard& ezBlackboardComponent::GetBoard() const
{
  return *m_pBoard.Borrow();
}

struct BCFlags
{
  enum Enum
  {
    ShowDebugInfo = 0
  };
};

void ezBlackboardComponent::SetShowDebugInfo(bool bShow)
{
  SetUserFlag(BCFlags::ShowDebugInfo, bShow);

  if (IsActiveAndInitialized())
  {
    GetOwner()->UpdateLocalBounds();
  }
}

bool ezBlackboardComponent::GetShowDebugInfo() const
{
  return GetUserFlag(BCFlags::ShowDebugInfo);
}

void ezBlackboardComponent::OnUpdateLocalBounds(ezMsgUpdateLocalBounds& msg) const
{
  if (GetShowDebugInfo())
  {
    msg.AddBounds(ezBoundingSphere(ezVec3::ZeroVector(), 2.0f), ezDefaultSpatialDataCategories::RenderDynamic);
  }
}

void ezBlackboardComponent::OnExtractRenderData(ezMsgExtractRenderData& msg) const
{
  if (!GetShowDebugInfo())
    return;

  if (msg.m_pView->GetCameraUsageHint() != ezCameraUsageHint::MainView &&
      msg.m_pView->GetCameraUsageHint() != ezCameraUsageHint::EditorView)
    return;

  // Don't extract render data for selection.
  if (msg.m_OverrideCategory != ezInvalidRenderDataCategory)
    return;

  auto& entries = m_pBoard->GetAllEntries();
  if (entries.IsEmpty())
    return;

  ezStringBuilder sb;

  for (auto it = entries.GetIterator(); it.IsValid(); ++it)
  {
    sb.AppendFormat("{}: {}\n", it.Key(), it.Value().m_Value);
  }

  ezDebugRenderer::Draw3DText(msg.m_pView->GetHandle(), sb, GetOwner()->GetGlobalPosition(), ezColor::Orange, 16, ezDebugRenderer::HorizontalAlignment::Center, ezDebugRenderer::VerticalAlignment::Bottom);
}

ezUInt32 ezBlackboardComponent::Entries_GetCount() const
{
  return m_InitialEntries.GetCount();
}

const ezBlackboardEntry& ezBlackboardComponent::Entries_GetValue(ezUInt32 uiIndex) const
{
  return m_InitialEntries[uiIndex];
}

void ezBlackboardComponent::Entries_SetValue(ezUInt32 uiIndex, const ezBlackboardEntry& entry)
{
  m_InitialEntries.EnsureCount(uiIndex + 1);
  m_InitialEntries[uiIndex] = entry;

  RegisterEntries(m_InitialEntries);
}

void ezBlackboardComponent::Entries_Insert(ezUInt32 uiIndex, const ezBlackboardEntry& entry)
{
  m_InitialEntries.Insert(entry, uiIndex);

  RegisterEntries(m_InitialEntries);
}

void ezBlackboardComponent::Entries_Remove(ezUInt32 uiIndex)
{
  m_InitialEntries.RemoveAtAndCopy(uiIndex);

  RegisterEntries(m_InitialEntries);
}

void ezBlackboardComponent::RegisterEntries(ezArrayPtr<ezBlackboardEntry> entries)
{
  m_pBoard->UnregisterAllEntries();

  for (auto& entry : entries)
  {
    m_pBoard->RegisterEntry(entry.m_sName, entry.m_InitialValue, entry.m_Flags);
  }
}
