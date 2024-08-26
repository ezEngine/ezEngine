#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/StateMachineAsset/StateMachineGraph.h>
#include <EditorPluginAssets/StateMachineAsset/StateMachineGraphQt.moc.h>
#include <SharedPluginAssets/StateMachineAsset/StateMachineGraphTypes.h>

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

constexpr const char* s_szIsInitialState = "IsInitialState";

ezStateMachineNodeManager::ezStateMachineNodeManager()
{
  m_StructureEvents.AddEventHandler(ezMakeDelegate(&ezStateMachineNodeManager::StructureEventHandler, this));
}

ezStateMachineNodeManager::~ezStateMachineNodeManager()
{
  m_StructureEvents.RemoveEventHandler(ezMakeDelegate(&ezStateMachineNodeManager::StructureEventHandler, this));
}

bool ezStateMachineNodeManager::IsInitialState(const ezDocumentObject* pObject) const
{
  return pObject->GetTypeAccessor().GetValue(s_szIsInitialState) == true;
}

const ezDocumentObject* ezStateMachineNodeManager::GetInitialState() const
{
  for (auto pObject : GetRootObject()->GetChildren())
  {
    if (IsNode(pObject) && IsInitialState(pObject))
    {
      return pObject;
    }
  }

  return nullptr;
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

void ezStateMachineNodeManager::StructureEventHandler(const ezDocumentObjectStructureEvent& e)
{
  if (IsNode(e.m_pObject) == false || IsAnyState(e.m_pObject))
    return;

  auto pCommandHistory = GetDocument()->GetCommandHistory();
  if (pCommandHistory == nullptr || pCommandHistory->IsInTransaction() == false)
    return;

  if (e.m_EventType == ezDocumentObjectStructureEvent::Type::AfterObjectAdded &&
      e.m_pObject->GetTypeAccessor().GetValue(s_szIsInitialState) == false &&
      GetInitialState() == nullptr)
  {
    ezSetObjectPropertyCommand propCmd;
    propCmd.m_Object = e.m_pObject->GetGuid();
    propCmd.m_sProperty = s_szIsInitialState;
    propCmd.m_NewValue = ezVariant(true);

    EZ_VERIFY(pCommandHistory->AddCommand(propCmd).Succeeded(), "");
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
    if (m_NewInitialStateObject.IsValid())
      m_pNewInitialStateObject = pManager->GetObject(m_NewInitialStateObject);

    if (auto pOldInitialStateObject = pManager->GetInitialState())
      m_pOldInitialStateObject = pManager->GetObject(pOldInitialStateObject->GetGuid());
  }

  if (m_pNewInitialStateObject)
    EZ_SUCCEED_OR_RETURN(pDocument->GetObjectManager()->SetValue(m_pNewInitialStateObject, s_szIsInitialState, ezVariant(true)));

  if (m_pOldInitialStateObject)
    EZ_SUCCEED_OR_RETURN(pDocument->GetObjectManager()->SetValue(m_pOldInitialStateObject, s_szIsInitialState, ezVariant(false)));

  return ezStatus(EZ_SUCCESS);
}

ezStatus ezStateMachine_SetInitialStateCommand::UndoInternal(bool bFireEvents)
{
  ezDocument* pDocument = GetDocument();
  auto pManager = static_cast<ezStateMachineNodeManager*>(pDocument->GetObjectManager());

  if (m_pNewInitialStateObject)
    EZ_SUCCEED_OR_RETURN(pDocument->GetObjectManager()->SetValue(m_pNewInitialStateObject, s_szIsInitialState, ezVariant(false)));

  if (m_pOldInitialStateObject)
    EZ_SUCCEED_OR_RETURN(pDocument->GetObjectManager()->SetValue(m_pOldInitialStateObject, s_szIsInitialState, ezVariant(true)));

  return ezStatus(EZ_SUCCESS);
}
