#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/StateMachineAsset/StateMachineGraph.h>
#include <EditorPluginAssets/StateMachineAsset/StateMachineGraphQt.moc.h>
#include <GameEngine/StateMachine/StateMachine.h>
#include <GuiFoundation/UIServices/DynamicStringEnum.h>

// clang-format off
EZ_BEGIN_SUBSYSTEM_DECLARATION(EditorPluginAssets, StateMachine)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "ReflectedTypeManager"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    ezQtNodeScene::GetPinFactory().RegisterCreator(ezGetStaticRTTI<ezStateMachinePin>(), [](const ezRTTI* pRtti)->ezQtPin* { return new ezQtStateMachinePin(); });
    ezQtNodeScene::GetConnectionFactory().RegisterCreator(ezGetStaticRTTI<ezStateMachineConnection>(), [](const ezRTTI* pRtti)->ezQtConnection* { return new ezQtStateMachineConnection(); });    
    ezQtNodeScene::GetNodeFactory().RegisterCreator(ezGetStaticRTTI<ezStateMachineNodeBase>(), [](const ezRTTI* pRtti)->ezQtNode* { return new ezQtStateMachineNode(); });
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    ezQtNodeScene::GetPinFactory().UnregisterCreator(ezGetStaticRTTI<ezStateMachinePin>());
    ezQtNodeScene::GetConnectionFactory().UnregisterCreator(ezGetStaticRTTI<ezStateMachineConnection>());
    ezQtNodeScene::GetNodeFactory().UnregisterCreator(ezGetStaticRTTI<ezStateMachineNodeBase>());
  }

EZ_END_SUBSYSTEM_DECLARATION;

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezStateMachinePin, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezStateMachinePin::ezStateMachinePin(Type type, const ezDocumentObject* pObject)
  : ezPin(type, type == Type::Input ? "Enter" : "Exit", ezColor::Grey, pObject)
{
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezStateMachineConnection, 1, ezRTTINoAllocator)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Type", m_pType)->AddFlags(ezPropertyFlags::PointerOwner)
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezStateMachineNodeBase, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezStateMachineNode, 1, ezRTTINoAllocator)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Name", m_sName)->AddAttributes(new ezDefaultValueAttribute(ezStringView("State"))), // wrap in ezStringView to prevent a memory leak report
    EZ_MEMBER_PROPERTY("Type", m_pType)->AddFlags(ezPropertyFlags::PointerOwner),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezStateMachineNodeAny, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

//////////////////////////////////////////////////////////////////////////

ezStateMachineNodeManager::ezStateMachineNodeManager()
{
  m_ObjectEvents.AddEventHandler(ezMakeDelegate(&ezStateMachineNodeManager::ObjectHandler, this));
}

ezStateMachineNodeManager::~ezStateMachineNodeManager()
{
  m_ObjectEvents.RemoveEventHandler(ezMakeDelegate(&ezStateMachineNodeManager::ObjectHandler, this));
}

void ezStateMachineNodeManager::SetInitialState(const ezDocumentObject* pObject)
{
  if (m_pInitialStateObject == pObject)
    return;

  EZ_ASSERT_DEV(IsAnyState(pObject) == false, "'Any State' can't be initial state");

  auto BroadcastEvent = [this](const ezDocumentObject* pObject) {
    if (pObject != nullptr)
    {
      ezDocumentObjectPropertyEvent e;
      e.m_EventType = ezDocumentObjectPropertyEvent::Type::PropertySet;
      e.m_pObject = pObject;

      m_PropertyEvents.Broadcast(e);
    }
  };

  const ezDocumentObject* pOldInitialStateObject = m_pInitialStateObject;
  m_pInitialStateObject = pObject;

  // Broadcast after the initial state object has been changed since the qt node will query it from the manager
  BroadcastEvent(pOldInitialStateObject);
  BroadcastEvent(m_pInitialStateObject);
}

bool ezStateMachineNodeManager::IsAnyState(const ezDocumentObject* pObject) const
{
  if (pObject != nullptr)
  {
    auto pType = pObject->GetTypeAccessor().GetType();
    return pType->IsDerivedFrom<ezStateMachineNodeAny>();
  }
  return false;
}

bool ezStateMachineNodeManager::InternalIsNode(const ezDocumentObject* pObject) const
{
  if (pObject != nullptr)
  {
    auto pType = pObject->GetTypeAccessor().GetType();
    return pType->IsDerivedFrom<ezStateMachineNodeBase>();
  }
  return false;
}

ezStatus ezStateMachineNodeManager::InternalCanConnect(const ezPin& source, const ezPin& target, CanConnectResult& out_Result) const
{
  out_Result = CanConnectResult::ConnectNtoN;
  return ezStatus(EZ_SUCCESS);
}

void ezStateMachineNodeManager::InternalCreatePins(const ezDocumentObject* pObject, NodeInternal& node)
{
  if (IsNode(pObject) == false)
    return;

  if (IsAnyState(pObject) == false)
  {
    auto pPin = EZ_DEFAULT_NEW(ezStateMachinePin, ezPin::Type::Input, pObject);
    node.m_Inputs.PushBack(pPin);
  }

  {
    auto pPin = EZ_DEFAULT_NEW(ezStateMachinePin, ezPin::Type::Output, pObject);
    node.m_Outputs.PushBack(pPin);
  }
}

void ezStateMachineNodeManager::GetCreateableTypes(ezHybridArray<const ezRTTI*, 32>& Types) const
{
  Types.PushBack(ezGetStaticRTTI<ezStateMachineNode>());
  Types.PushBack(ezGetStaticRTTI<ezStateMachineNodeAny>());
}

const ezRTTI* ezStateMachineNodeManager::GetConnectionType() const
{
  return ezGetStaticRTTI<ezStateMachineConnection>();
}

void ezStateMachineNodeManager::ObjectHandler(const ezDocumentObjectEvent& e)
{
  if (e.m_EventType == ezDocumentObjectEvent::Type::AfterObjectCreated && IsNode(e.m_pObject))
  {
    if (m_pInitialStateObject == nullptr && IsAnyState(e.m_pObject) == false)
    {
      SetInitialState(e.m_pObject);
    }
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezStateMachine_SetInitialStateCommand, 1, ezRTTIDefaultAllocator<ezStateMachine_SetInitialStateCommand>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("NewInitialStateObject", m_NewInitialStateObject),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezStateMachine_SetInitialStateCommand::ezStateMachine_SetInitialStateCommand() = default;

ezStatus ezStateMachine_SetInitialStateCommand::DoInternal(bool bRedo)
{
  ezDocument* pDocument = GetDocument();
  auto pManager = static_cast<ezStateMachineNodeManager*>(pDocument->GetObjectManager());

  if (!bRedo)
  {
    m_pNewInitialStateObject = pManager->GetObject(m_NewInitialStateObject);
    m_pOldInitialStateObject = pManager->GetInitialState();
  }

  pManager->SetInitialState(m_pNewInitialStateObject);
  return ezStatus(EZ_SUCCESS);
}

ezStatus ezStateMachine_SetInitialStateCommand::UndoInternal(bool bFireEvents)
{
  ezDocument* pDocument = GetDocument();
  auto pManager = static_cast<ezStateMachineNodeManager*>(pDocument->GetObjectManager());

  pManager->SetInitialState(m_pOldInitialStateObject);
  return ezStatus(EZ_SUCCESS);
}
