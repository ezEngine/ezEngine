#pragma once

#include <GameEngine/GameEngineDLL.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Types/Variant.h>
#include <Foundation/Containers/Map.h>
#include <GameEngine/VisualScript/VisualScriptNode.h>
#include <GameEngine/GameState/StateMap.h>
#include <Foundation/Containers/ArrayMap.h>
#include <Core/ResourceManager/ResourceHandle.h>

class ezVisualScriptNode;
class ezMessage;
struct ezVisualScriptResourceDescriptor;
class ezGameObject;
class ezWorld;
struct ezVisualScriptInstanceActivity;
struct ezEventMessage;

typedef ezUInt32 ezVisualScriptNodeConnectionID;
typedef ezUInt32 ezVisualScriptPinConnectionID;
typedef ezTypedResourceHandle<class ezVisualScriptResource> ezVisualScriptResourceHandle;

typedef bool(*ezVisualScriptDataPinAssignFunc)(const void* src, void* dst);

/// \brief An instance of a visual script resource. Stores the current script state and executes nodes.
class EZ_GAMEENGINE_DLL ezVisualScriptInstance
{
public:
  ezVisualScriptInstance();
  ~ezVisualScriptInstance();

  /// \brief Clears the current state and recreates the script instance from the given template.
  void Configure(const ezVisualScriptResourceHandle& hScript, ezGameObject* pOwner);

  /// \brief Runs all nodes that are marked for execution. Typically nodes that handle events will mark themselves for execution in the next update.
  void ExecuteScript(ezVisualScriptInstanceActivity* pActivity = nullptr);

  /// \brief The message is dispatched to all nodes, which may react on it, for instance by tagging themselves for execution in the next ExecuteScript() call.
  void HandleMessage(ezMessage& msg);

  /// \brief Called by ezVisualScriptNode classes to pass the new value of an output pin to all connected nodes.
  void SetOutputPinValue(const ezVisualScriptNode* pNode, ezUInt8 uiPin, const void* pValue);

  /// \brief Called by ezVisualScriptNode classes to execute the node that is connected on the given output execution pin.
  void ExecuteConnectedNodes(const ezVisualScriptNode* pNode, ezUInt16 uiNthTarget);

  /// \brief Returns the ezGameObject that owns this script. May be nullptr, if the instance is not attached to a game object.
  ezGameObjectHandle GetOwner() const { return m_hOwner; }

  /// \brief Returns the world of the owner game object.
  ezWorld* GetWorld() const { return m_pWorld; }

  /// \brief Returns the map that holds the local variables of the script.
  const ezStateMap& GetLocalVariables() const { return m_LocalVariables; }

  /// \brief Returns the map that holds the local variables of the script.
  ezStateMap& GetLocalVariables() { return m_LocalVariables; }

  /// \brief Needs to be called once to register the default data pin conversion functions.
  static void SetupPinDataTypeConversions();

  static void RegisterDataPinAssignFunction(ezVisualScriptDataPinType::Enum sourceType, ezVisualScriptDataPinType::Enum dstType, ezVisualScriptDataPinAssignFunc func);
  static ezVisualScriptDataPinAssignFunc FindDataPinAssignFunction(ezVisualScriptDataPinType::Enum sourceType, ezVisualScriptDataPinType::Enum dstType);

  /// \brief Returns whether this script has a node that handles this type of event message.
  bool HandlesEventMessage(const ezEventMessage& msg) const;

private:
  friend class ezVisualScriptNode;

  void Clear();
  void ComputeNodeDependencies();
  void ExecuteDependentNodes(ezUInt16 uiNode);

  void ConnectExecutionPins(ezUInt16 uiSourceNode, ezUInt8 uiOutputSlot, ezUInt16 uiTargetNode, ezUInt8 uiTargetPin);
  void ConnectDataPins(ezUInt16 uiSourceNode, ezUInt8 uiSourcePin, ezVisualScriptDataPinType::Enum sourcePinType, ezUInt16 uiTargetNode, ezUInt8 uiTargetPin, ezVisualScriptDataPinType::Enum targetPinType);

  void CreateVisualScriptNode(ezUInt32 uiNodeIdx, const ezVisualScriptResourceDescriptor& resource);
  void CreateFunctionMessageNode(ezUInt32 uiNodeIdx, const ezVisualScriptResourceDescriptor& resource);
  void CreateEventMessageNode(ezUInt32 uiNodeIdx, const ezVisualScriptResourceDescriptor& resource);
  void CreateFunctionCallNode(ezUInt32 uiNodeIdx, const ezVisualScriptResourceDescriptor& resource);
  ezAbstractFunctionProperty* SearchForScriptableFunctionOnType(const ezRTTI* pObjectType, ezStringView sFuncName, const ezScriptableFunctionAttribute*& out_pSfAttr) const;

  struct DataPinConnection
  {
    EZ_DECLARE_POD_TYPE();

    ezUInt16 m_uiTargetNode;
    ezUInt8 m_uiTargetPin;
    ezVisualScriptDataPinAssignFunc m_AssignFunc = nullptr;
    void* m_pTargetData = nullptr;
  };

  struct ExecPinConnection
  {
    EZ_DECLARE_POD_TYPE();

    ezUInt16 m_uiTargetNode;
    ezUInt8 m_uiTargetPin;
  };

  ezVisualScriptResourceHandle m_hScriptResource;
  ezGameObjectHandle m_hOwner;
  ezWorld* m_pWorld = nullptr;
  ezDynamicArray<ezVisualScriptNode*> m_Nodes;
  ezDynamicArray<ezHybridArray<ezUInt16, 2>> m_NodeDependencies;
  ezHashTable<ezVisualScriptNodeConnectionID, ExecPinConnection > m_ExecutionConnections;
  ezHashTable<ezVisualScriptPinConnectionID, ezHybridArray<DataPinConnection, 2> > m_DataConnections;
  ezStateMap m_LocalVariables;
  ezVisualScriptInstanceActivity* m_pActivity = nullptr;
  const ezArrayMap<ezMessageId, ezUInt16>* m_pMessageHandlers = nullptr;

  struct AssignFuncKey
  {
    EZ_DECLARE_POD_TYPE();

    ezVisualScriptDataPinType::Enum m_SourceType;
    ezVisualScriptDataPinType::Enum m_DstType;

    EZ_ALWAYS_INLINE bool operator==(const AssignFuncKey& rhs) const
    {
      return m_SourceType == rhs.m_SourceType && m_DstType == rhs.m_DstType;
    }

    EZ_ALWAYS_INLINE bool operator<(const AssignFuncKey& rhs) const
    {
      if (m_SourceType < rhs.m_SourceType)
        return true;
      if (m_SourceType > rhs.m_SourceType)
        return false;

      return m_DstType < rhs.m_DstType;
    }
  };

  static ezMap<AssignFuncKey, ezVisualScriptDataPinAssignFunc> s_DataPinAssignFunctions;
};


struct EZ_GAMEENGINE_DLL ezVisualScriptInstanceActivity
{
  ezHybridArray<ezUInt32, 16> m_ActiveExecutionConnections;
  ezHybridArray<ezUInt32, 16> m_ActiveDataConnections;

  void Clear()
  {
    m_ActiveDataConnections.Clear();
    m_ActiveExecutionConnections.Clear();
  }

  bool IsEmpty()
  {
    return m_ActiveDataConnections.IsEmpty() && m_ActiveExecutionConnections.IsEmpty();
  }
};

