#include <GameComponentsPlugin/GameComponentsPCH.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/Messages/SetColorMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameComponentsPlugin/Gameplay/PowerConnectorComponent.h>

// clang-format off
EZ_IMPLEMENT_MESSAGE_TYPE(ezEventMsgSetPowerInput);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezEventMsgSetPowerInput, 1, ezRTTIDefaultAllocator<ezEventMsgSetPowerInput>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("PrevValue", m_uiPrevValue),
    EZ_MEMBER_PROPERTY("NewValue", m_uiNewValue),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_COMPONENT_TYPE(ezPowerConnectorComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Output", GetOutput, SetOutput),
    EZ_ACCESSOR_PROPERTY("Buddy", DummyGetter, SetBuddyReference)->AddAttributes(new ezGameObjectReferenceAttribute()),
    EZ_ACCESSOR_PROPERTY("ConnectedTo", DummyGetter, SetConnectedToReference)->AddAttributes(new ezGameObjectReferenceAttribute()),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgSensorDetectedObjectsChanged, OnMsgSensorDetectedObjectsChanged),
    EZ_MESSAGE_HANDLER(ezMsgObjectGrabbed, OnMsgObjectGrabbed),
  }
  EZ_END_MESSAGEHANDLERS;
  EZ_BEGIN_FUNCTIONS
  {
    EZ_SCRIPT_FUNCTION_PROPERTY(IsConnected),
    EZ_SCRIPT_FUNCTION_PROPERTY(IsAttached),
    EZ_SCRIPT_FUNCTION_PROPERTY(Detach),
    // EZ_SCRIPT_FUNCTION_PROPERTY(Attach, In, "Object"), // not supported (yet)
  }
  EZ_END_FUNCTIONS;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Gameplay"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE;
// clang-format on

void ezPowerConnectorComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();

  inout_stream.WriteGameObjectHandle(m_hBuddy);
  inout_stream.WriteGameObjectHandle(m_hConnectedTo);

  s << m_uiOutput;
}

void ezPowerConnectorComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();

  m_hBuddy = inout_stream.ReadGameObjectHandle();
  m_hConnectedTo = inout_stream.ReadGameObjectHandle();

  s >> m_uiOutput;
}

void ezPowerConnectorComponent::ConnectToSocket(ezGameObjectHandle hSocket)
{
  if (IsConnected())
    return;

  if (GetOwner()->GetWorld()->GetClock().GetAccumulatedTime() - m_DetachTime < ezTime::MakeFromSeconds(1))
  {
    // recently detached -> wait a bit before allowing to attach again
    return;
  }

  Attach(hSocket);
}

void ezPowerConnectorComponent::SetOutput(ezUInt16 value)
{
  if (m_uiOutput == value)
    return;

  m_uiOutput = value;

  OutputChanged(m_uiOutput);
}

void ezPowerConnectorComponent::SetInput(ezUInt16 value)
{
  if (m_uiInput == value)
    return;

  InputChanged(m_uiInput, value);
  m_uiInput = value;
}

void ezPowerConnectorComponent::SetBuddyReference(const char* szReference)
{
  auto resolver = GetWorld()->GetGameObjectReferenceResolver();

  if (!resolver.IsValid())
    return;

  SetBuddy(resolver(szReference, GetHandle(), "Buddy"));
}

void ezPowerConnectorComponent::SetBuddy(ezGameObjectHandle hNewBuddy)
{
  if (m_hBuddy == hNewBuddy)
    return;

  if (!IsActiveAndInitialized())
  {
    m_hBuddy = hNewBuddy;
    return;
  }

  ezGameObjectHandle hPrevBuddy = m_hBuddy;
  m_hBuddy = {};

  ezGameObject* pBuddy;
  if (GetOwner()->GetWorld()->TryGetObject(hPrevBuddy, pBuddy))
  {
    ezPowerConnectorComponent* pConnector;
    if (pBuddy->TryGetComponentOfBaseType(pConnector))
    {
      pConnector->SetOutput(0);
      pConnector->SetBuddy({});
    }
  }

  m_hBuddy = hNewBuddy;

  if (GetOwner()->GetWorld()->TryGetObject(hNewBuddy, pBuddy))
  {
    ezPowerConnectorComponent* pConnector;
    if (pBuddy->TryGetComponentOfBaseType(pConnector))
    {
      pConnector->SetBuddy(GetOwner()->GetHandle());
      pConnector->SetOutput(m_uiInput);
    }
  }
}

void ezPowerConnectorComponent::SetConnectedToReference(const char* szReference)
{
  auto resolver = GetWorld()->GetGameObjectReferenceResolver();

  if (!resolver.IsValid())
    return;

  SetConnectedTo(resolver(szReference, GetHandle(), "ConnectedTo"));
}

void ezPowerConnectorComponent::SetConnectedTo(ezGameObjectHandle hNewConnectedTo)
{
  if (m_hConnectedTo == hNewConnectedTo)
    return;

  if (!IsActiveAndInitialized())
  {
    m_hConnectedTo = hNewConnectedTo;
    return;
  }

  ezGameObjectHandle hPrevConnectedTo = m_hConnectedTo;
  m_hConnectedTo = {};

  ezGameObject* pConnectedTo;
  if (GetOwner()->GetWorld()->TryGetObject(hPrevConnectedTo, pConnectedTo))
  {
    ezPowerConnectorComponent* pConnector;
    if (pConnectedTo->TryGetComponentOfBaseType(pConnector))
    {
      pConnector->SetInput(0);
      pConnector->SetConnectedTo({});
    }
  }

  m_hConnectedTo = hNewConnectedTo;

  if (GetOwner()->GetWorld()->TryGetObject(hNewConnectedTo, pConnectedTo))
  {
    ezPowerConnectorComponent* pConnector;
    if (pConnectedTo->TryGetComponentOfBaseType(pConnector))
    {
      pConnector->SetConnectedTo(GetOwner()->GetHandle());
      pConnector->SetInput(m_uiOutput);
    }
  }

  if (hNewConnectedTo.IsInvalidated() && IsAttached())
  {
    // make sure that if we get disconnected, we also clean up our detachment state
    Detach();
  }
}

bool ezPowerConnectorComponent::IsConnected() const
{
  // since connectors automatically disconnect themselves from their peers upon destruction, this should be sufficient (no need to check object for existence)
  return !m_hConnectedTo.IsInvalidated();
}

bool ezPowerConnectorComponent::IsAttached() const
{
  return !m_hAttachPoint.IsInvalidated();
}

void ezPowerConnectorComponent::OnDeactivated()
{
  Detach();
  SetBuddy({});

  SUPER::OnDeactivated();
}

void ezPowerConnectorComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  ezGameObjectHandle hAlreadyConnectedTo = m_hConnectedTo;
  m_hConnectedTo.Invalidate();

  if (!hAlreadyConnectedTo.IsInvalidated())
  {
    Attach(hAlreadyConnectedTo);
  }

  if (m_uiInput != 0)
  {
    InputChanged(0, m_uiInput);
  }

  if (m_uiOutput != 0)
  {
    OutputChanged(m_uiOutput);
  }
}

void ezPowerConnectorComponent::OnMsgSensorDetectedObjectsChanged(ezMsgSensorDetectedObjectsChanged& msg)
{
  if (!msg.m_DetectedObjects.IsEmpty())
  {
    ConnectToSocket(msg.m_DetectedObjects[0]);
  }
}

void ezPowerConnectorComponent::OnMsgObjectGrabbed(ezMsgObjectGrabbed& msg)
{
  if (msg.m_bGotGrabbed)
  {
    Detach();

    m_hGrabbedBy = msg.m_hGrabbedBy;

    if (ezGameObject* pSensor = GetOwner()->FindChildByName("ActiveWhenGrabbed"))
    {
      pSensor->SetActiveFlag(true);
    }
  }
  else
  {
    m_hGrabbedBy.Invalidate();

    if (ezGameObject* pSensor = GetOwner()->FindChildByName("ActiveWhenGrabbed"))
    {
      pSensor->SetActiveFlag(false);
    }
  }
}

void ezPowerConnectorComponent::Attach(ezGameObjectHandle hSocket)
{
  ezWorld* pWorld = GetOwner()->GetWorld();

  ezGameObject* pSocket;
  if (!pWorld->TryGetObject(hSocket, pSocket))
    return;

  ezPowerConnectorComponent* pConnector;
  if (pSocket->TryGetComponentOfBaseType(pConnector))
  {
    // don't connect to an already connected object
    if (pConnector->IsConnected())
      return;
  }

  const ezTransform tSocket = pSocket->GetGlobalTransform();

  ezGameObjectDesc go;
  go.m_hParent = hSocket;

  ezGameObject* pAttach;
  m_hAttachPoint = pWorld->CreateObject(go, pAttach);

  ezPhysicsWorldModuleInterface* pPhysicsWorldModule = GetWorld()->GetOrCreateModule<ezPhysicsWorldModuleInterface>();

  ezPhysicsWorldModuleInterface::FixedJointConfig cfg;
  cfg.m_hActorA = {};
  cfg.m_hActorB = GetOwner()->GetHandle();
  cfg.m_LocalFrameA = tSocket;
  pPhysicsWorldModule->AddFixedJointComponent(pAttach, cfg);

  SetConnectedTo(hSocket);

  if (!m_hGrabbedBy.IsInvalidated())
  {
    ezGameObject* pGrab;
    if (pWorld->TryGetObject(m_hGrabbedBy, pGrab))
    {
      ezMsgReleaseObjectGrab msg;
      msg.m_hGrabbedObjectToRelease = GetOwner()->GetHandle();
      pGrab->SendMessage(msg);
    }
  }
}

void ezPowerConnectorComponent::Detach()
{
  if (IsConnected())
  {
    m_DetachTime = GetOwner()->GetWorld()->GetClock().GetAccumulatedTime();

    SetConnectedTo({});
  }

  if (!m_hAttachPoint.IsInvalidated())
  {
    GetOwner()->GetWorld()->DeleteObjectDelayed(m_hAttachPoint, false);
    m_hAttachPoint.Invalidate();
  }
}

void ezPowerConnectorComponent::InputChanged(ezUInt16 uiPrevInput, ezUInt16 uiInput)
{
  if (!IsActiveAndSimulating())
    return;

  ezEventMsgSetPowerInput msg;
  msg.m_uiPrevValue = uiPrevInput;
  msg.m_uiNewValue = uiInput;

  GetOwner()->PostEventMessage(msg, this, ezTime());

  if (m_hBuddy.IsInvalidated())
    return;

  ezGameObject* pBuddy;
  if (GetOwner()->GetWorld()->TryGetObject(m_hBuddy, pBuddy))
  {
    ezPowerConnectorComponent* pConnector;
    if (pBuddy->TryGetComponentOfBaseType(pConnector))
    {
      pConnector->SetOutput(uiInput);
    }
  }
}

void ezPowerConnectorComponent::OutputChanged(ezUInt16 uiOutput)
{
  if (!IsActiveAndSimulating())
    return;

  if (m_hConnectedTo.IsInvalidated())
    return;

  ezGameObject* pConnectedTo;
  if (GetOwner()->GetWorld()->TryGetObject(m_hConnectedTo, pConnectedTo))
  {
    ezPowerConnectorComponent* pConnector;
    if (pConnectedTo->TryGetComponentOfBaseType(pConnector))
    {
      pConnector->SetInput(uiOutput);
    }
  }
}


EZ_STATICLINK_FILE(GameComponentsPlugin, GameComponentsPlugin_Gameplay_Implementation_PowerConnectorComponent);

