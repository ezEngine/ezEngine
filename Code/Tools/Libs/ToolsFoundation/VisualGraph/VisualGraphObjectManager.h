#pragma once

#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Types/Status.h>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class ezVisualGraphPin;
class ezVisualGraphConnection;

/// Event structure for visual graph changes
struct EZ_TOOLSFOUNDATION_DLL ezVisualGraphObjectManagerEvent
{
  enum class Type
  {
    NodeMoved,
    AfterPinsConnected,
    BeforePinsDisonnected,
    BeforePinsChanged,
    AfterPinsChanged,
    BeforeNodeAdded,
    AfterNodeAdded,
    BeforeNodeRemoved,
    AfterNodeRemoved,
  };

  ezVisualGraphObjectManagerEvent(Type eventType, const ezDocumentObject* pObject = nullptr)
    : m_EventType(eventType)
    , m_pObject(pObject)
  {
  }

  Type m_EventType;
  const ezDocumentObject* m_pObject;
};

/// Represents an active connection between two pins in a visual graph.
///
/// This class is created and managed by ezVisualGraphObjectManager when pins are connected.
/// It holds references to both the source and target pins.
class ezVisualGraphConnection final
{
public:
  const ezVisualGraphPin& GetSourcePin() const { return m_SourcePin; }
  const ezVisualGraphPin& GetTargetPin() const { return m_TargetPin; }
  const ezDocumentObject* GetParent() const { return m_pParent; }

private:
  friend class ezVisualGraphObjectManager;

  ezVisualGraphConnection(const ezVisualGraphPin& sourcePin, const ezVisualGraphPin& targetPin, const ezDocumentObject* pParent)
    : m_SourcePin(sourcePin)
    , m_TargetPin(targetPin)
    , m_pParent(pParent)
  {
  }

  const ezVisualGraphPin& m_SourcePin;
  const ezVisualGraphPin& m_TargetPin;
  const ezDocumentObject* m_pParent = nullptr;
};

/// Represents a connection point (input or output) on a visual graph node.
///
/// Pins are created by ezVisualGraphObjectManager for each node based on the node type.
/// Derived classes can extend pins with additional metadata specific to their graph type.
class EZ_TOOLSFOUNDATION_DLL ezVisualGraphPin : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisualGraphPin, ezReflectedClass);

public:
  enum class Type
  {
    Input,
    Output
  };

  enum class Shape
  {
    Circle,
    Rect,
    RoundRect,
    Arrow,
    Default = Circle
  };

  ezVisualGraphPin(Type type, ezStringView sName, const ezColorGammaUB& color, const ezDocumentObject* pObject)
    : m_Type(type)
    , m_Color(color)
    , m_sName(sName)
    , m_pParent(pObject)
  {
  }

  Shape m_Shape = Shape::Default;

  Type GetType() const { return m_Type; }
  const char* GetName() const { return m_sName; }
  const ezColorGammaUB& GetColor() const { return m_Color; }
  const ezDocumentObject* GetParent() const { return m_pParent; }

private:
  friend class ezVisualGraphObjectManager;

  Type m_Type;
  ezColorGammaUB m_Color;
  ezString m_sName;
  const ezDocumentObject* m_pParent = nullptr;
};

//////////////////////////////////////////////////////////////////////////

/// A property value that can be pre-filled when creating a node from a template
struct ezVisualGraphNodeProperty
{
  ezHashedString m_sPropertyName;
  ezVariant m_Value;
};

/// \brief Describes a template that will be used to create new nodes. In most cases this only contains the type
/// but it can also contain properties that are pre-filled when the node is created.
///
/// For example in visual script this allows us to have one generic node type for setting reflected properties
/// but we can expose all relevant reflected properties in the node creation menu so the user does not need to fill out the property name manually.
struct ezVisualGraphNodeDesc
{
  const ezRTTI* m_pType = nullptr;
  ezStringView m_sTypeName;
  ezHashedString m_sCategory;
  ezArrayPtr<const ezVisualGraphNodeProperty> m_PropertyValues;
};

//////////////////////////////////////////////////////////////////////////

/// Serializable base class for storing connection data in documents.
///
/// Derive from this class and overwrite ezVisualGraphObjectManager::GetConnectionType if you need custom properties for connections.
/// This class stores the persistent connection information, while ezVisualGraphConnection holds runtime connection references.
class EZ_TOOLSFOUNDATION_DLL ezDocumentObject_ConnectionBase : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezDocumentObject_ConnectionBase, ezReflectedClass);

public:
  ezUuid m_Source;
  ezUuid m_Target;
  ezString m_SourcePin;
  ezString m_TargetPin;
};

//////////////////////////////////////////////////////////////////////////

/// Document object manager for node-based visual graphs.
///
/// This base class manages the document-side representation of visual graphs, including nodes, pins, and connections.
/// It handles node creation, pin management, connection validation, and provides events for graph changes.
/// Derive from this class to implement specific graph types such as visual shaders, state machines, or visual scripts.
/// The visual representation is handled separately by ezQtVisualGraphScene.
class EZ_TOOLSFOUNDATION_DLL ezVisualGraphObjectManager : public ezDocumentObjectManager
{
public:
  ezEvent<const ezVisualGraphObjectManagerEvent&> m_NodeEvents;

  ezVisualGraphObjectManager();
  virtual ~ezVisualGraphObjectManager();

  /// \brief For node documents this function is called instead of GetCreateableTypes to get a list for the node creation menu.
  ///
  /// \see ezVisualGraphNodeDesc
  virtual void GetNodeCreationTemplates(ezDynamicArray<ezVisualGraphNodeDesc>& out_templates) const;

  virtual const ezRTTI* GetConnectionType() const;

  ezVec2 GetNodePos(const ezDocumentObject* pObject) const;
  const ezVisualGraphConnection& GetConnection(const ezDocumentObject* pObject) const;
  const ezVisualGraphConnection* GetConnectionIfExists(const ezDocumentObject* pObject) const;

  const ezVisualGraphPin* GetInputPinByName(const ezDocumentObject* pObject, ezStringView sName) const;
  const ezVisualGraphPin* GetOutputPinByName(const ezDocumentObject* pObject, ezStringView sName) const;
  ezArrayPtr<const ezUniquePtr<const ezVisualGraphPin>> GetInputPins(const ezDocumentObject* pObject) const;
  ezArrayPtr<const ezUniquePtr<const ezVisualGraphPin>> GetOutputPins(const ezDocumentObject* pObject) const;

  /// Specifies how many connections are allowed for a pair of pins
  enum class CanConnectResult
  {
    ConnectNever, ///< Pins can't be connected
    Connect1to1,  ///< Output pin can have 1 outgoing connection, Input pin can have 1 incoming connection
    Connect1toN,  ///< Output pin can have 1 outgoing connection, Input pin can have N incoming connections
    ConnectNto1,  ///< Output pin can have N outgoing connections, Input pin can have 1 incoming connection
    ConnectNtoN,  ///< Output pin can have N outgoing connections, Input pin can have N incoming connections
  };

  bool IsNode(const ezDocumentObject* pObject) const;
  bool IsConnection(const ezDocumentObject* pObject) const;
  bool IsDynamicPinProperty(const ezDocumentObject* pObject, const ezAbstractProperty* pProp) const;

  ezArrayPtr<const ezVisualGraphConnection* const> GetConnections(const ezVisualGraphPin& pin) const;
  bool HasConnections(const ezVisualGraphPin& pin) const;
  bool IsConnected(const ezVisualGraphPin& source, const ezVisualGraphPin& target) const;

  ezStatus CanConnect(const ezRTTI* pObjectType, const ezVisualGraphPin& source, const ezVisualGraphPin& target, CanConnectResult& ref_result) const;
  ezStatus CanDisconnect(const ezVisualGraphConnection* pConnection) const;
  ezStatus CanDisconnect(const ezDocumentObject* pObject) const;
  ezStatus CanMoveNode(const ezDocumentObject* pObject, const ezVec2& vPos) const;

  void Connect(const ezDocumentObject* pObject, const ezVisualGraphPin& source, const ezVisualGraphPin& target);
  void Disconnect(const ezDocumentObject* pObject);
  void MoveNode(const ezDocumentObject* pObject, const ezVec2& vPos);

  void AttachMetaDataBeforeSaving(ezAbstractObjectGraph& ref_graph) const;
  void RestoreMetaDataAfterLoading(const ezAbstractObjectGraph& graph, bool bUndoable);

  void GetMetaDataHash(const ezDocumentObject* pObject, ezUInt64& inout_uiHash) const;
  bool CopySelectedObjects(ezAbstractObjectGraph& out_objectGraph) const;
  bool PasteObjects(const ezArrayPtr<ezDocument::PasteInfo>& info, const ezAbstractObjectGraph& objectGraph, const ezVec2& vPickedPosition, bool bAllowPickedPosition);

protected:
  /// \brief Tests whether pTarget can be reached from pSource by following the pin connections
  bool CanReachNode(const ezDocumentObject* pSource, const ezDocumentObject* pTarget, ezSet<const ezDocumentObject*>& Visited) const;

  /// \brief Returns true if adding a connection between the two pins would create a circular graph
  bool WouldConnectionCreateCircle(const ezVisualGraphPin& source, const ezVisualGraphPin& target) const;

  ezResult ResolveConnection(const ezUuid& sourceObject, const ezUuid& targetObject, ezStringView sourcePin, ezStringView targetPin, const ezVisualGraphPin*& out_pSourcePin, const ezVisualGraphPin*& out_pTargetPin) const;

  virtual void GetDynamicPinNames(const ezDocumentObject* pObject, ezStringView sPropertyName, ezStringView sPinName, ezDynamicArray<ezString>& out_Names) const;
  virtual bool TryRecreatePins(const ezDocumentObject* pObject);

  struct NodeInternal
  {
    ezVec2 m_vPos = ezVec2::MakeZero();
    ezHybridArray<ezUniquePtr<ezVisualGraphPin>, 6> m_Inputs;
    ezHybridArray<ezUniquePtr<ezVisualGraphPin>, 6> m_Outputs;
  };

private:
  virtual bool InternalIsNode(const ezDocumentObject* pObject) const;
  virtual bool InternalIsConnection(const ezDocumentObject* pObject) const;
  virtual bool InternalIsDynamicPinProperty(const ezDocumentObject* pObject, const ezAbstractProperty* pProp) const { return false; }
  virtual ezStatus InternalCanConnect(const ezVisualGraphPin& source, const ezVisualGraphPin& target, CanConnectResult& out_Result) const;
  virtual ezStatus InternalCanDisconnect(const ezVisualGraphPin& source, const ezVisualGraphPin& target) const { return ezStatus(EZ_SUCCESS); }
  virtual ezStatus InternalCanMoveNode(const ezDocumentObject* pObject, const ezVec2& vPos) const { return ezStatus(EZ_SUCCESS); }
  virtual void InternalCreatePins(const ezDocumentObject* pObject, NodeInternal& node) = 0;

  void ObjectHandler(const ezDocumentObjectEvent& e);
  void StructureEventHandler(const ezDocumentObjectStructureEvent& e);
  void PropertyEventsHandler(const ezDocumentObjectPropertyEvent& e);

  void HandlePotentialDynamicPinPropertyChanged(const ezDocumentObject* pObject, ezStringView sPropertyName);

private:
  ezHashTable<ezUuid, NodeInternal> m_ObjectToNode;
  ezHashTable<ezUuid, ezUniquePtr<ezVisualGraphConnection>> m_ObjectToConnection;
  ezMap<const ezVisualGraphPin*, ezHybridArray<const ezVisualGraphConnection*, 6>> m_Connections;
};
