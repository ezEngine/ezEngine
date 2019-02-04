#include <PCH.h>

#include <Core/World/EventMessageHandlerComponent.h>
#include <Core/World/World.h>

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

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezEventMessageHandlerComponent, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezEventMessageHandlerComponent::ezEventMessageHandlerComponent()
    : m_bDebugOutput(false)
    , m_bIsGlobalEventHandler(false)
{
}

ezEventMessageHandlerComponent::~ezEventMessageHandlerComponent() {}

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

bool ezEventMessageHandlerComponent::HandlesEventMessage(const ezEventMessage& msg) const
{
  return true;
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



EZ_STATICLINK_FILE(Core, Core_World_Implementation_EventMessageHandlerComponent);

