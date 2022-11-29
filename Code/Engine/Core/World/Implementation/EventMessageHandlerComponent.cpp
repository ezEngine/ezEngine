#include <Core/CorePCH.h>

#include <Core/World/EventMessageHandlerComponent.h>
#include <Core/World/World.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>

namespace
{
  static ezStaticArray<ezDynamicArray<ezComponentHandle>*, 64> s_GlobalEventHandlerPerWorld;

  static void RegisterGlobalEventHandler(ezComponent* pComponent)
  {
    const ezUInt32 uiWorldIndex = pComponent->GetWorld()->GetIndex();
    s_GlobalEventHandlerPerWorld.EnsureCount(uiWorldIndex + 1);

    auto globalEventHandler = s_GlobalEventHandlerPerWorld[uiWorldIndex];
    if (globalEventHandler == nullptr)
    {
      globalEventHandler = EZ_NEW(ezStaticAllocatorWrapper::GetAllocator(), ezDynamicArray<ezComponentHandle>);

      s_GlobalEventHandlerPerWorld[uiWorldIndex] = globalEventHandler;
    }

    globalEventHandler->PushBack(pComponent->GetHandle());
  }

  static void DeregisterGlobalEventHandler(ezComponent* pComponent)
  {
    ezUInt32 uiWorldIndex = pComponent->GetWorld()->GetIndex();
    auto globalEventHandler = s_GlobalEventHandlerPerWorld[uiWorldIndex];
    EZ_ASSERT_DEV(globalEventHandler != nullptr, "Implementation error.");

    globalEventHandler->RemoveAndSwap(pComponent->GetHandle());
  }
} // namespace

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_ABSTRACT_COMPONENT_TYPE(ezEventMessageHandlerComponent, 3)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("HandleGlobalEvents", GetGlobalEventHandlerMode, SetGlobalEventHandlerMode),
    EZ_ACCESSOR_PROPERTY("PassThroughUnhandledEvents", GetPassThroughUnhandledEvents, SetPassThroughUnhandledEvents),
  }
  EZ_END_PROPERTIES;
}
EZ_END_ABSTRACT_COMPONENT_TYPE;
// clang-format on

ezEventMessageHandlerComponent::ezEventMessageHandlerComponent() = default;
ezEventMessageHandlerComponent::~ezEventMessageHandlerComponent() = default;

void ezEventMessageHandlerComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  auto& s = stream.GetStream();

  // version 2
  s << m_bIsGlobalEventHandler;

  // version 3
  s << m_bPassThroughUnhandledEvents;
}

void ezEventMessageHandlerComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = stream.GetStream();

  if (uiVersion >= 2)
  {
    bool bGlobalEH;
    s >> bGlobalEH;

    SetGlobalEventHandlerMode(bGlobalEH);
  }

  if (uiVersion >= 3)
  {
    s >> m_bPassThroughUnhandledEvents;
  }
}

void ezEventMessageHandlerComponent::Deinitialize()
{
  SetGlobalEventHandlerMode(false);

  SUPER::Deinitialize();
}

void ezEventMessageHandlerComponent::SetDebugOutput(bool enable)
{
  m_bDebugOutput = enable;
}

bool ezEventMessageHandlerComponent::GetDebugOutput() const
{
  return m_bDebugOutput;
}

void ezEventMessageHandlerComponent::SetGlobalEventHandlerMode(bool enable)
{
  if (m_bIsGlobalEventHandler == enable)
    return;

  m_bIsGlobalEventHandler = enable;

  if (enable)
  {
    RegisterGlobalEventHandler(this);
  }
  else
  {
    DeregisterGlobalEventHandler(this);
  }
}

void ezEventMessageHandlerComponent::SetPassThroughUnhandledEvents(bool bPassThrough)
{
  m_bPassThroughUnhandledEvents = bPassThrough;
}

bool ezEventMessageHandlerComponent::HandlesEventMessage(const ezEventMessage& msg) const
{
  return m_pMessageDispatchType->CanHandleMessage(msg.GetId());
}

// static
ezArrayPtr<ezComponentHandle> ezEventMessageHandlerComponent::GetAllGlobalEventHandler(const ezWorld* pWorld)
{
  ezUInt32 uiWorldIndex = pWorld->GetIndex();

  if (uiWorldIndex < s_GlobalEventHandlerPerWorld.GetCount())
  {
    if (auto globalEventHandler = s_GlobalEventHandlerPerWorld[uiWorldIndex])
    {
      return globalEventHandler->GetArrayPtr();
    }
  }

  return ezArrayPtr<ezComponentHandle>();
}


void ezEventMessageHandlerComponent::ClearGlobalEventHandlersForWorld(const ezWorld* pWorld)
{
  ezUInt32 uiWorldIndex = pWorld->GetIndex();

  if (uiWorldIndex < s_GlobalEventHandlerPerWorld.GetCount())
  {
    s_GlobalEventHandlerPerWorld[uiWorldIndex]->Clear();
  }
}

EZ_STATICLINK_FILE(Core, Core_World_Implementation_EventMessageHandlerComponent);
