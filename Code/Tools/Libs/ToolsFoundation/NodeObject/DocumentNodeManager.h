#pragma once

#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Types/Status.h>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class ezPin;
class ezConnection;

struct EZ_TOOLSFOUNDATION_DLL ezDocumentNodeManagerEvent
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

  ezDocumentNodeManagerEvent(Type eventType, const ezDocumentObject* pObject = nullptr)
    : m_EventType(eventType)
    , m_pObject(pObject)
  {
  }

  Type m_EventType;
  const ezDocumentObject* m_pObject;
};

class ezConnection
{
public:
  const ezPin& GetSourcePin() const { return m_SourcePin; }
  const ezPin& GetTargetPin() const { return m_TargetPin; }
  const ezDocumentObject* GetParent() const { return m_pParent; }

private:
  friend class ezDocumentNodeManager;

  ezConnection(const ezPin& sourcePin, const ezPin& targetPin, const ezDocumentObject* pParent)
    : m_SourcePin(sourcePin)
    , m_TargetPin(targetPin)
    , m_pParent(pParent)
  {
  }

  const ezPin& m_SourcePin;
  const ezPin& m_TargetPin;
  const ezDocumentObject* m_pParent = nullptr;
};

class EZ_TOOLSFOUNDATION_DLL ezPin : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezPin, ezReflectedClass);

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

  ezPin(Type type, ezStringView sName, const ezColorGammaUB& color, const ezDocumentObject* pObject)
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
  friend class ezDocumentNodeManager;

  Type m_Type;
  ezColorGammaUB m_Color;
  ezString m_sName;
  const ezDocumentObject* m_pParent = nullptr;
};

//////////////////////////////////////////////////////////////////////////

struct ezNodePropertyValue
{
  ezHashedString m_sPropertyName;
  ezVariant m_Value;
};

/// \brief Describes a template that will be used to create new nodes. In most cases this only contains the type
/// but it can also contain properties that are pre-filled when the node is created.
///
/// For example in visual script this allows us to have one generic node type for setting reflected properties
/// but we can expose all relevant reflected properties in the node creation menu so the user does not need to fill out the property name manually.
struct ezNodeCreationTemplate
{
  const ezRTTI* m_pType = nullptr;
  ezStringView m_sTypeName;
  ezHashedString m_sCategory;
  ezArrayPtr<const ezNodePropertyValue> m_PropertyValues;
};

//////////////////////////////////////////////////////////////////////////

/// \brief Base class for all node connections. Derive from this class and overwrite ezDocumentNodeManager::GetConnectionType
/// if you need custom properties for connections.
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

class EZ_TOOLSFOUNDATION_DLL ezDocumentNodeManager : public ezDocumentObjectManager
{
public:
  ezEvent<const ezDocumentNodeManagerEvent&> m_NodeEvents;

  ezDocumentNodeManager();
  virtual ~ezDocumentNodeManager();

  /// \brief For node documents this function is called instead of GetCreateableTypes to get a list for the node creation menu.
  ///
  /// \see ezNodeCreationTemplate
  virtual void GetNodeCreationTemplates(ezDynamicArray<ezNodeCreationTemplate>& out_templates) const;

  virtual const ezRTTI* GetConnectionType() const;

  ezVec2 GetNodePos(const ezDocumentObject* pObject) const;
  const ezConnection& GetConnection(const ezDocumentObject* pObject) const;
  const ezConnection* GetConnectionIfExists(const ezDocumentObject* pObject) const;

  const ezPin* GetInputPinByName(const ezDocumentObject* pObject, ezStringView sName) const;
  const ezPin* GetOutputPinByName(const ezDocumentObject* pObject, ezStringView sName) const;
  ezArrayPtr<const ezUniquePtr<const ezPin>> GetInputPins(const ezDocumentObject* pObject) const;
  ezArrayPtr<const ezUniquePtr<const ezPin>> GetOutputPins(const ezDocumentObject* pObject) const;

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

  ezArrayPtr<const ezConnection* const> GetConnections(const ezPin& pin) const;
  bool HasConnections(const ezPin& pin) const;
  bool IsConnected(const ezPin& source, const ezPin& target) const;

  ezStatus CanConnect(const ezRTTI* pObjectType, const ezPin& source, const ezPin& target, CanConnectResult& ref_result) const;
  ezStatus CanDisconnect(const ezConnection* pConnection) const;
  ezStatus CanDisconnect(const ezDocumentObject* pObject) const;
  ezStatus CanMoveNode(const ezDocumentObject* pObject, const ezVec2& vPos) const;

  void Connect(const ezDocumentObject* pObject, const ezPin& source, const ezPin& target);
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
  bool WouldConnectionCreateCircle(const ezPin& source, const ezPin& target) const;

  ezResult ResolveConnection(const ezUuid& sourceObject, const ezUuid& targetObject, ezStringView sourcePin, ezStringView targetPin, const ezPin*& out_pSourcePin, const ezPin*& out_pTargetPin) const;

  virtual void GetDynamicPinNames(const ezDocumentObject* pObject, ezStringView sPropertyName, ezStringView sPinName, ezDynamicArray<ezString>& out_Names) const;
  virtual bool TryRecreatePins(const ezDocumentObject* pObject);

  struct NodeInternal
  {
    ezVec2 m_vPos = ezVec2::MakeZero();
    ezHybridArray<ezUniquePtr<ezPin>, 6> m_Inputs;
    ezHybridArray<ezUniquePtr<ezPin>, 6> m_Outputs;
  };

private:
  virtual bool InternalIsNode(const ezDocumentObject* pObject) const;
  virtual bool InternalIsConnection(const ezDocumentObject* pObject) const;
  virtual bool InternalIsDynamicPinProperty(const ezDocumentObject* pObject, const ezAbstractProperty* pProp) const { return false; }
  virtual ezStatus InternalCanConnect(const ezPin& source, const ezPin& target, CanConnectResult& out_Result) const;
  virtual ezStatus InternalCanDisconnect(const ezPin& source, const ezPin& target) const { return ezStatus(EZ_SUCCESS); }
  virtual ezStatus InternalCanMoveNode(const ezDocumentObject* pObject, const ezVec2& vPos) const { return ezStatus(EZ_SUCCESS); }
  virtual void InternalCreatePins(const ezDocumentObject* pObject, NodeInternal& node) = 0;

  void ObjectHandler(const ezDocumentObjectEvent& e);
  void StructureEventHandler(const ezDocumentObjectStructureEvent& e);
  void PropertyEventsHandler(const ezDocumentObjectPropertyEvent& e);

  void HandlePotentialDynamicPinPropertyChanged(const ezDocumentObject* pObject, ezStringView sPropertyName);

private:
  ezHashTable<ezUuid, NodeInternal> m_ObjectToNode;
  ezHashTable<ezUuid, ezUniquePtr<ezConnection>> m_ObjectToConnection;
  ezMap<const ezPin*, ezHybridArray<const ezConnection*, 6>> m_Connections;
};
