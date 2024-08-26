#pragma once

#include <SharedPluginAssets/SharedPluginAssetsDLL.h>

#include <ToolsFoundation/NodeObject/DocumentNodeManager.h>

class ezStateMachineState;
class ezStateMachineTransition;

/// \brief A connection that represents a state machine transition. Since we can't chose different connection
/// types in the Editor we allow the user to switch the type in the properties.
class EZ_SHAREDPLUGINASSETS_DLL ezStateMachineConnection : public ezDocumentObject_ConnectionBase
{
  EZ_ADD_DYNAMIC_REFLECTION(ezStateMachineConnection, ezDocumentObject_ConnectionBase);

public:
  ezStateMachineTransition* m_pType = nullptr;
};

/// \brief Base class for nodes in the state machine graph
class EZ_SHAREDPLUGINASSETS_DLL ezStateMachineNodeBase : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezStateMachineNodeBase, ezReflectedClass);
};

/// \brief A node that represents a state machine state. We don't use ezStateMachineState directly to allow
/// the user to switch the type in the properties similar to what we do with transitions.
class EZ_SHAREDPLUGINASSETS_DLL ezStateMachineNode : public ezStateMachineNodeBase
{
  EZ_ADD_DYNAMIC_REFLECTION(ezStateMachineNode, ezStateMachineNodeBase);

public:
  ezString m_sName;
  ezStateMachineState* m_pType = nullptr;
  bool m_bIsInitialState = false;
};

/// \brief A node that represents "any" state machine state. This can be used if a transition with the same conditions
/// is possible from any other state in the state machine. Instead of creating many connections with the same properties
/// an "any" state can be used to make the graph much easier to read and to maintain.
///
/// Note that there is no "any" state at runtime but rather only the transition is stored.
class EZ_SHAREDPLUGINASSETS_DLL ezStateMachineNodeAny : public ezStateMachineNodeBase
{
  EZ_ADD_DYNAMIC_REFLECTION(ezStateMachineNodeAny, ezStateMachineNodeBase);
};
