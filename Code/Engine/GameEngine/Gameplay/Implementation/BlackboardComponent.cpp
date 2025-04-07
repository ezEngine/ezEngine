#include "Foundation/Serialization/AbstractObjectGraph.h"

#include <GameEngine/GameEnginePCH.h>

#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Serialization/GraphPatch.h>
#include <GameEngine/Gameplay/BlackboardComponent.h>
#include <GameEngine/Utils/BlackboardTemplateResource.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Pipeline/View.h>

struct BCFlags
{
  enum Enum
  {
    ShowDebugInfo = 0,
    SendEntryChangedMessage,
    InitializedFromTemplate
  };
};

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_TYPE(ezBlackboardEntry, ezNoBase, 1, ezRTTIDefaultAllocator<ezBlackboardEntry>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Name", m_sName)->AddAttributes(new ezDynamicStringEnumAttribute("BlackboardKeysEnum")),
    EZ_MEMBER_PROPERTY("InitialValue", m_InitialValue)->AddAttributes(new ezDefaultValueAttribute(0)),
    EZ_BITFLAGS_MEMBER_PROPERTY("Flags", ezBlackboardEntryFlags, m_Flags)
  }
  EZ_END_PROPERTIES;
}
EZ_END_STATIC_REFLECTED_TYPE;
// clang-format on

ezResult ezBlackboardEntry::Serialize(ezStreamWriter& inout_stream) const
{
  inout_stream << m_sName;
  inout_stream << m_InitialValue;
  inout_stream << m_Flags;

  return EZ_SUCCESS;
}

ezResult ezBlackboardEntry::Deserialize(ezStreamReader& inout_stream)
{
  inout_stream >> m_sName;
  inout_stream >> m_InitialValue;
  inout_stream >> m_Flags;

  return EZ_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_IMPLEMENT_MESSAGE_TYPE(ezMsgBlackboardEntryChanged);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMsgBlackboardEntryChanged, 1, ezRTTIDefaultAllocator<ezMsgBlackboardEntryChanged>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Name", GetName, SetName),
    EZ_MEMBER_PROPERTY("OldValue", m_OldValue),
    EZ_MEMBER_PROPERTY("NewValue", m_NewValue),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_ABSTRACT_COMPONENT_TYPE(ezBlackboardComponent, 3)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_RESOURCE_MEMBER_PROPERTY("Template", m_hTemplate)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_BlackboardTemplate")),
    EZ_ACCESSOR_PROPERTY("ShowDebugInfo", GetShowDebugInfo, SetShowDebugInfo),
  }
  EZ_END_PROPERTIES;

  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgUpdateLocalBounds, OnUpdateLocalBounds),
    EZ_MESSAGE_HANDLER(ezMsgExtractRenderData, OnExtractRenderData),
  }
  EZ_END_MESSAGEHANDLERS;

  EZ_BEGIN_FUNCTIONS
  {
    EZ_SCRIPT_FUNCTION_PROPERTY(Reflection_FindBlackboard, In, "SearchObject", In, "BlackboardName")->AddFlags(ezPropertyFlags::PureFunction)->AddAttributes(new ezFunctionArgumentAttributes(1, new ezDynamicStringEnumAttribute("BlackboardNamesEnum"))),
    EZ_SCRIPT_FUNCTION_PROPERTY(SetEntryValue, In, "Name", In, "Value")->AddAttributes(new ezFunctionArgumentAttributes(0, new ezDynamicStringEnumAttribute("BlackboardKeysEnum"))),
    EZ_SCRIPT_FUNCTION_PROPERTY(GetEntryValue, In, "Name")->AddAttributes(new ezFunctionArgumentAttributes(0, new ezDynamicStringEnumAttribute("BlackboardKeysEnum"))),
  }
  EZ_END_FUNCTIONS;

  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Logic"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_ABSTRACT_COMPONENT_TYPE;
// clang-format on

ezBlackboardComponent::ezBlackboardComponent() = default;
ezBlackboardComponent::~ezBlackboardComponent() = default;

// static
ezSharedPtr<ezBlackboard> ezBlackboardComponent::FindBlackboard(ezGameObject* pObject, ezStringView sBlackboardName /*= ezStringView()*/)
{
  const ezTempHashedString sBlackboardNameHashed(sBlackboardName);

  ezBlackboardComponent* pBlackboardComponent = nullptr;
  while (pObject != nullptr)
  {
    if (pObject->TryGetComponentOfBaseType(pBlackboardComponent))
    {
      if (sBlackboardName.IsEmpty() || (pBlackboardComponent->GetBoard() && pBlackboardComponent->GetBoard()->GetNameHashed() == sBlackboardNameHashed))
      {
        return pBlackboardComponent->GetBoard();
      }
    }

    pObject = pObject->GetParent();
  }

  if (sBlackboardName.IsEmpty() == false)
  {
    ezHashedString sHashedBlackboardName;
    sHashedBlackboardName.Assign(sBlackboardName);
    return ezBlackboard::GetOrCreateGlobal(sHashedBlackboardName);
  }

  return nullptr;
}

void ezBlackboardComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  ezStreamWriter& s = inout_stream.GetStream();

  s << m_hTemplate;
}

void ezBlackboardComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const ezUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  if (uiVersion < 3)
    return;

  ezStreamReader& s = inout_stream.GetStream();

  s >> m_hTemplate;
}

void ezBlackboardComponent::OnActivated()
{
  SUPER::OnActivated();

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

  SUPER::OnDeactivated();
}

const ezSharedPtr<ezBlackboard>& ezBlackboardComponent::GetBoard()
{
  return m_pBoard;
}

ezSharedPtr<const ezBlackboard> ezBlackboardComponent::GetBoard() const
{
  return m_pBoard;
}

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

void ezBlackboardComponent::SetEntryValue(const char* szName, const ezVariant& value)
{
  if (m_pBoard)
  {
    m_pBoard->SetEntryValue(szName, value);
  }
}

ezVariant ezBlackboardComponent::GetEntryValue(const char* szName) const
{
  if (m_pBoard)
  {
    return m_pBoard->GetEntryValue(ezTempHashedString(szName));
  }

  return {};
}

// static
ezBlackboard* ezBlackboardComponent::Reflection_FindBlackboard(ezGameObject* pSearchObject, ezStringView sBlackboardName)
{
  return FindBlackboard(pSearchObject, sBlackboardName).Borrow();
}

void ezBlackboardComponent::OnUpdateLocalBounds(ezMsgUpdateLocalBounds& msg) const
{
  if (GetShowDebugInfo())
  {
    msg.AddBounds(ezBoundingSphere::MakeFromCenterAndRadius(ezVec3::MakeZero(), 2.0f), ezDefaultSpatialDataCategories::RenderDynamic);
  }
}

void ezBlackboardComponent::OnExtractRenderData(ezMsgExtractRenderData& msg) const
{
  if (!GetShowDebugInfo() || m_pBoard == nullptr)
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
  sb.Append(m_pBoard->GetName(), "\n");

  for (auto it = entries.GetIterator(); it.IsValid(); ++it)
  {
    sb.AppendFormat("{}: {}\n", it.Key(), it.Value().m_Value);
  }

  ezDebugRenderer::Draw3DText(msg.m_pView->GetHandle(), sb, GetOwner()->GetGlobalPosition(), ezColor::Orange);
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezLocalBlackboardComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("BlackboardName", GetBlackboardName, SetBlackboardName)->AddAttributes(new ezDynamicStringEnumAttribute("BlackboardNamesEnum")),
    EZ_ACCESSOR_PROPERTY("SendEntryChangedMessage", GetSendEntryChangedMessage, SetSendEntryChangedMessage),
    EZ_ARRAY_ACCESSOR_PROPERTY("Entries", Entries_GetCount, Entries_GetValue, Entries_SetValue, Entries_Insert, Entries_Remove),
  }
  EZ_END_PROPERTIES;

  EZ_BEGIN_MESSAGESENDERS
  {
    EZ_MESSAGE_SENDER(m_EntryChangedSender)
  }
  EZ_END_MESSAGESENDERS;
}
EZ_END_DYNAMIC_REFLECTED_TYPE
// clang-format on

ezLocalBlackboardComponent::ezLocalBlackboardComponent()
{
  m_pBoard = ezBlackboard::Create();
}

ezLocalBlackboardComponent::ezLocalBlackboardComponent(ezLocalBlackboardComponent&& other) = default;
ezLocalBlackboardComponent::~ezLocalBlackboardComponent() = default;
ezLocalBlackboardComponent& ezLocalBlackboardComponent::operator=(ezLocalBlackboardComponent&& other) = default;

void ezLocalBlackboardComponent::Initialize()
{
  SUPER::Initialize();

  if (IsActive())
  {
    // we already do this here, so that the BB is initialized even if OnSimulationStarted() hasn't been called yet
    InitializeFromTemplate();
    SetUserFlag(BCFlags::InitializedFromTemplate, true);
  }
}

void ezLocalBlackboardComponent::OnActivated()
{
  SUPER::OnActivated();

  if (GetUserFlag(BCFlags::InitializedFromTemplate) == false)
  {
    InitializeFromTemplate();
  }
}

void ezLocalBlackboardComponent::OnDeactivated()
{
  SUPER::OnDeactivated();

  SetUserFlag(BCFlags::InitializedFromTemplate, false);
}

void ezLocalBlackboardComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  // we repeat this here, mainly for the editor case, when the asset has been modified (new entries added)
  // and we then press play, to have the new entries in the BB
  // this would NOT update the initial values, though, if they changed
  InitializeFromTemplate();

  // override with the component specific initial entries
  for (auto& entry : m_InitialEntries)
  {
    m_pBoard->SetEntryValue(entry.m_sName, entry.m_InitialValue);
    m_pBoard->SetEntryFlags(entry.m_sName, entry.m_Flags).AssertSuccess();
  }
}

void ezLocalBlackboardComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  ezStreamWriter& s = inout_stream.GetStream();

  s << m_pBoard->GetName();
  s.WriteArray(m_InitialEntries).IgnoreResult();
}

void ezLocalBlackboardComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  const ezUInt32 uiBaseVersion = inout_stream.GetComponentTypeVersion(ezBlackboardComponent::GetStaticRTTI());
  if (uiBaseVersion < 3)
    return;

  SUPER::DeserializeComponent(inout_stream);
  const ezUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  ezStreamReader& s = inout_stream.GetStream();

  ezStringBuilder sb;
  s >> sb;
  m_pBoard->SetName(sb);
  m_pBoard->RemoveAllEntries();

  // we don't write the data to m_InitialEntries, because that is never needed anymore at runtime
  ezDynamicArray<ezBlackboardEntry> initialEntries;
  if (s.ReadArray(initialEntries).Succeeded())
  {
    for (auto& entry : initialEntries)
    {
      m_pBoard->SetEntryValue(entry.m_sName, entry.m_InitialValue);
      m_pBoard->SetEntryFlags(entry.m_sName, entry.m_Flags).AssertSuccess();
    }
  }
}

void ezLocalBlackboardComponent::SetSendEntryChangedMessage(bool bSend)
{
  if (GetSendEntryChangedMessage() == bSend)
    return;

  SetUserFlag(BCFlags::SendEntryChangedMessage, bSend);

  if (bSend)
  {
    m_pBoard->OnEntryEvent().AddEventHandler(ezMakeDelegate(&ezLocalBlackboardComponent::OnEntryChanged, this));
  }
  else
  {
    m_pBoard->OnEntryEvent().RemoveEventHandler(ezMakeDelegate(&ezLocalBlackboardComponent::OnEntryChanged, this));
  }
}

bool ezLocalBlackboardComponent::GetSendEntryChangedMessage() const
{
  return GetUserFlag(BCFlags::SendEntryChangedMessage);
}

void ezLocalBlackboardComponent::SetBlackboardName(const char* szName)
{
  m_pBoard->SetName(szName);
}

const char* ezLocalBlackboardComponent::GetBlackboardName() const
{
  return m_pBoard->GetName();
}


ezUInt32 ezLocalBlackboardComponent::Entries_GetCount() const
{
  return m_InitialEntries.GetCount();
}

const ezBlackboardEntry& ezLocalBlackboardComponent::Entries_GetValue(ezUInt32 uiIndex) const
{
  return m_InitialEntries[uiIndex];
}

void ezLocalBlackboardComponent::Entries_SetValue(ezUInt32 uiIndex, const ezBlackboardEntry& entry)
{
  m_InitialEntries.EnsureCount(uiIndex + 1);

  if (const ezBlackboard::Entry* pEntry = m_pBoard->GetEntry(m_InitialEntries[uiIndex].m_sName))
  {
    if (m_InitialEntries[uiIndex].m_sName != entry.m_sName)
    {
      m_pBoard->RemoveEntry(m_InitialEntries[uiIndex].m_sName);
    }
  }

  m_pBoard->SetEntryValue(entry.m_sName, entry.m_InitialValue);
  m_pBoard->SetEntryFlags(entry.m_sName, entry.m_Flags).AssertSuccess();

  m_InitialEntries[uiIndex] = entry;
}

void ezLocalBlackboardComponent::Entries_Insert(ezUInt32 uiIndex, const ezBlackboardEntry& entry)
{
  m_InitialEntries.InsertAt(uiIndex, entry);

  m_pBoard->SetEntryValue(entry.m_sName, entry.m_InitialValue);
  m_pBoard->SetEntryFlags(entry.m_sName, entry.m_Flags).AssertSuccess();
}

void ezLocalBlackboardComponent::Entries_Remove(ezUInt32 uiIndex)
{
  auto& entry = m_InitialEntries[uiIndex];
  m_pBoard->RemoveEntry(entry.m_sName);

  m_InitialEntries.RemoveAtAndCopy(uiIndex);
}

void ezLocalBlackboardComponent::OnEntryChanged(const ezBlackboard::EntryEvent& e)
{
  if (!IsActiveAndInitialized())
    return;

  ezMsgBlackboardEntryChanged msg;
  msg.m_sName = e.m_sName;
  msg.m_OldValue = e.m_OldValue;
  msg.m_NewValue = e.m_pEntry->m_Value;

  m_EntryChangedSender.SendEventMessage(msg, this, GetOwner());
}

void ezLocalBlackboardComponent::InitializeFromTemplate()
{
  if (!m_hTemplate.IsValid())
    return;

  ezResourceLock<ezBlackboardTemplateResource> pTemplate(m_hTemplate, ezResourceAcquireMode::BlockTillLoaded_NeverFail);

  if (pTemplate.GetAcquireResult() != ezResourceAcquireResult::Final)
    return;

  for (const auto& entry : pTemplate->GetDescriptor().m_Entries)
  {
    m_pBoard->SetEntryValue(entry.m_sName, entry.m_InitialValue);
    m_pBoard->SetEntryFlags(entry.m_sName, entry.m_Flags).AssertSuccess();
  }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_ENUM(ezGlobalBlackboardInitMode, 1)
  EZ_ENUM_CONSTANTS(ezGlobalBlackboardInitMode::EnsureEntriesExist, ezGlobalBlackboardInitMode::ResetEntryValues, ezGlobalBlackboardInitMode::ClearEntireBlackboard)
EZ_END_STATIC_REFLECTED_ENUM;

EZ_BEGIN_COMPONENT_TYPE(ezGlobalBlackboardComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("BlackboardName", GetBlackboardName, SetBlackboardName)->AddAttributes(new ezDynamicStringEnumAttribute("BlackboardNamesEnum")),
    EZ_ENUM_MEMBER_PROPERTY("InitMode", ezGlobalBlackboardInitMode, m_InitMode),
  }
  EZ_END_PROPERTIES;

  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Logic"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE
// clang-format on

ezGlobalBlackboardComponent::ezGlobalBlackboardComponent() = default;
ezGlobalBlackboardComponent::ezGlobalBlackboardComponent(ezGlobalBlackboardComponent&& other) = default;
ezGlobalBlackboardComponent::~ezGlobalBlackboardComponent() = default;
ezGlobalBlackboardComponent& ezGlobalBlackboardComponent::operator=(ezGlobalBlackboardComponent&& other) = default;

void ezGlobalBlackboardComponent::Initialize()
{
  SUPER::Initialize();

  if (IsActive())
  {
    // we already do this here, so that the BB is initialized even if OnSimulationStarted() hasn't been called yet
    InitializeFromTemplate();
    SetUserFlag(BCFlags::InitializedFromTemplate, true);
  }
}

void ezGlobalBlackboardComponent::OnActivated()
{
  SUPER::OnActivated();

  if (GetUserFlag(BCFlags::InitializedFromTemplate) == false)
  {
    InitializeFromTemplate();
  }
}

void ezGlobalBlackboardComponent::OnDeactivated()
{
  SUPER::OnDeactivated();

  SetUserFlag(BCFlags::InitializedFromTemplate, false);
}

void ezGlobalBlackboardComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  // we repeat this here, mainly for the editor case, when the asset has been modified (new entries added)
  // and we then press play, to have the new entries in the BB
  // this would NOT update the initial values, though, if they changed
  InitializeFromTemplate();
}

void ezGlobalBlackboardComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  ezStreamWriter& s = inout_stream.GetStream();

  s << m_sName;
  s << m_InitMode;
}

void ezGlobalBlackboardComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  const ezUInt32 uiBaseVersion = inout_stream.GetComponentTypeVersion(ezBlackboardComponent::GetStaticRTTI());
  if (uiBaseVersion < 3)
    return;

  SUPER::DeserializeComponent(inout_stream);
  const ezUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  ezStreamReader& s = inout_stream.GetStream();

  s >> m_sName;
  s >> m_InitMode;
}

void ezGlobalBlackboardComponent::SetBlackboardName(const char* szName)
{
  m_sName.Assign(szName);
}

const char* ezGlobalBlackboardComponent::GetBlackboardName() const
{
  return m_sName;
}

void ezGlobalBlackboardComponent::InitializeFromTemplate()
{
  m_pBoard = ezBlackboard::GetOrCreateGlobal(m_sName);

  if (m_InitMode == ezGlobalBlackboardInitMode::ClearEntireBlackboard)
  {
    m_pBoard->RemoveAllEntries();
  }

  if (!m_hTemplate.IsValid())
    return;

  ezResourceLock<ezBlackboardTemplateResource> pTemplate(m_hTemplate, ezResourceAcquireMode::BlockTillLoaded_NeverFail);

  if (pTemplate.GetAcquireResult() != ezResourceAcquireResult::Final)
    return;

  for (const auto& entry : pTemplate->GetDescriptor().m_Entries)
  {
    if (!m_pBoard->HasEntry(entry.m_sName) || m_InitMode != ezGlobalBlackboardInitMode::EnsureEntriesExist)
    {
      // make sure the entry exists and enforce that it has this value
      m_pBoard->SetEntryValue(entry.m_sName, entry.m_InitialValue);
      // also overwrite the flags
      m_pBoard->SetEntryFlags(entry.m_sName, entry.m_Flags).AssertSuccess();
    }
  }
}


class ezBlackboardComponent_2_3 : public ezGraphPatch
{
public:
  ezBlackboardComponent_2_3()
    : ezGraphPatch("ezBlackboardComponent", 3)
  {
  }

  virtual void Patch(ezGraphPatchContext& ref_context, ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
  {
    ref_context.RenameClass("ezLocalBlackboardComponent");
  }
};

ezBlackboardComponent_2_3 g_ezBlackboardComponent_2_3;


EZ_STATICLINK_FILE(GameEngine, GameEngine_Gameplay_Implementation_BlackboardComponent);
