#pragma once

#include <SharedPluginAssets/SharedPluginAssetsDLL.h>

#include <ToolsFoundation/VisualGraph/VisualGraphObjectManager.h>

class ezStateMachineState;
class ezStateMachineTransition;

/// Connection representing a state machine transition.
///
/// Since the editor doesn't support choosing different connection types, users can switch the transition type
/// in the properties panel instead.
class EZ_SHAREDPLUGINASSETS_DLL ezStateMachineConnection : public ezDocumentObject_ConnectionBase
{
  EZ_ADD_DYNAMIC_REFLECTION(ezStateMachineConnection, ezDocumentObject_ConnectionBase);

public:
  ezStateMachineTransition* m_pType = nullptr;
};

/// Base class for nodes in a state machine graph.
class EZ_SHAREDPLUGINASSETS_DLL ezStateMachineNodeBase : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezStateMachineNodeBase, ezReflectedClass);
};

/// Node representing a state machine state.
///
/// Does not use ezStateMachineState directly to allow users to switch the state type in the properties panel,
/// similar to transition handling.
class EZ_SHAREDPLUGINASSETS_DLL ezStateMachineNode : public ezStateMachineNodeBase
{
  EZ_ADD_DYNAMIC_REFLECTION(ezStateMachineNode, ezStateMachineNodeBase);

public:
  ezString m_sName;
  ezStateMachineState* m_pType = nullptr;
  bool m_bIsInitialState = false;
};

/// Node representing the "any state" in a state machine.
///
/// Used when a transition with the same conditions is possible from any other state.
/// Instead of creating many identical connections, an "any state" node simplifies the graph and maintenance.
/// At runtime, there is no "any state" object - only the transitions are stored.
class EZ_SHAREDPLUGINASSETS_DLL ezStateMachineNodeAny : public ezStateMachineNodeBase
{
  EZ_ADD_DYNAMIC_REFLECTION(ezStateMachineNodeAny, ezStateMachineNodeBase);
};
