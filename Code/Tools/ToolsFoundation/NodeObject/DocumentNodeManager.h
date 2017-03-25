#pragma once

#include <ToolsFoundation/Basics.h>
#include <ToolsFoundation/Basics/Status.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class ezAbstractObjectGraph;
class ezDocumentObject;
class ezPin;
class ezConnection;

struct EZ_TOOLSFOUNDATION_DLL ezDocumentNodeManagerEvent
{
  enum class Type
  {
    NodeMoved,
    AfterPinsConnected,
    BeforePinsDisonnected,
    BeforePinsChanged,// todo
    AfterPinsChanged,// todo
    BeforeNodeAdded,
    AfterNodeAdded,
    BeforeNodeRemoved,
    AfterNodeRemoved,
  };

  ezDocumentNodeManagerEvent(Type eventType, const ezDocumentObject* pObject = nullptr, const ezConnection* pConnection = nullptr)
    : m_EventType(eventType), m_pObject(pObject), m_pConnection(pConnection)
  {}

  Type m_EventType;
  const ezDocumentObject* m_pObject;
  const ezConnection* m_pConnection;
};

class EZ_TOOLSFOUNDATION_DLL ezConnection : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezConnection, ezReflectedClass);
public:
  const ezPin* GetSourcePin() const { return m_pSourcePin; }
  const ezPin* GetTargetPin() const { return m_pTargetPin; }

private:
  friend class ezDocumentNodeManager;

  const ezPin* m_pSourcePin;
  const ezPin* m_pTargetPin;
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

  ezPin(Type type, const char* szName, const ezDocumentObject* pObject)
    : m_Type(type), m_sName(szName), m_pParent(pObject)
  {
  }

  Type GetType() const { return m_Type; }
  const char* GetName() const {return m_sName;}
  const ezDocumentObject* GetParent() const { return m_pParent; }
  const ezArrayPtr<const ezConnection* const> GetConnections() const { return m_Connections; }

private:
  friend class ezDocumentNodeManager;

  Type m_Type;
  ezString m_sName;
  const ezDocumentObject* m_pParent;
  ezHybridArray<const ezConnection*, 6> m_Connections;
};

class EZ_TOOLSFOUNDATION_DLL ezDocumentNodeManager : public ezDocumentObjectManager
{
public:
  ezEvent<const ezDocumentNodeManagerEvent&> m_NodeEvents;

  ezDocumentNodeManager();
  virtual ~ezDocumentNodeManager();

  virtual void DestroyAllObjects() override;

  ezVec2 GetNodePos(const ezDocumentObject* pObject) const;
  const ezPin* GetInputPinByName(const ezDocumentObject* pObject, const char* szName) const;
  const ezPin* GetOutputPinByName(const ezDocumentObject* pObject, const char* szName) const;
  const ezArrayPtr<ezPin* const> GetInputPins(const ezDocumentObject* pObject) const;
  const ezArrayPtr<ezPin* const> GetOutputPins(const ezDocumentObject* pObject) const;
  const ezConnection* GetConnection(const ezPin* pSource, const ezPin* pTarget) const;

  enum class CanConnectResult
  {
    ConnectNever,
    Connect1to1,
    Connect1toN,
    ConnectNto1,
    ConnectNtoN,
  };

  bool IsNode(const ezDocumentObject* pObject) const;
  ezStatus CanConnect(const ezPin* pSource, const ezPin* pTarget, CanConnectResult& result) const;
  ezStatus CanDisconnect(const ezConnection* pConnection) const;
  ezStatus CanDisconnect(const ezPin* pSource, const ezPin* pTarget) const;
  ezStatus CanMoveNode(const ezDocumentObject* pObject, const ezVec2& vPos) const;

  void Connect(const ezPin* pSource, const ezPin* pTarget);
  void Disconnect(const ezPin* pSource, const ezPin* pTarget);
  void MoveNode(const ezDocumentObject* pObject, const ezVec2& vPos);

  void AttachMetaDataBeforeSaving(ezAbstractObjectGraph& graph) const;
  void RestoreMetaDataAfterLoading(const ezAbstractObjectGraph& graph);

protected:
  /// \brief Tests whether pTarget can be reached from pSource by following the pin connections
  bool CanReachNode(const ezDocumentObject* pSource, const ezDocumentObject* pTarget, ezSet<const ezDocumentObject*>& Visited) const;

  /// \brief Returns true if adding a connection between the two pins would create a circular graph
  bool WouldConnectionCreateCircle(const ezPin* pSource, const ezPin* pTarget) const;

  struct NodeInternal
  {
    NodeInternal() : m_vPos(0, 0) {}
    ezVec2 m_vPos;
    ezHybridArray<ezPin*, 6> m_Inputs;
    ezHybridArray<ezPin*, 6> m_Outputs;
  };
  struct PinTuple
  {
    PinTuple() : m_pSourcePin(nullptr), m_pTargetPin(nullptr) {}
    PinTuple(const ezPin* pPinOut, const ezPin* pPinIn) : m_pSourcePin(pPinOut), m_pTargetPin(pPinIn) {}

    bool operator< (const PinTuple& rhs) const
    {
      if (m_pSourcePin == rhs.m_pSourcePin)
      {
        return m_pTargetPin < rhs.m_pTargetPin;
      }
      else
      {
        return m_pSourcePin < rhs.m_pSourcePin;
      }
    }

    bool operator== (const PinTuple& rhs) const
    {
      return m_pSourcePin == rhs.m_pSourcePin && m_pTargetPin == rhs.m_pTargetPin;
    }

    const ezPin* m_pSourcePin;
    const ezPin* m_pTargetPin;
  };

private:
  virtual bool InternalIsNode(const ezDocumentObject* pObject) const { return true; }
  virtual ezStatus InternalCanConnect(const ezPin* pSource, const ezPin* pTarget, CanConnectResult& out_Result) const { out_Result = CanConnectResult::ConnectNtoN; return ezStatus(EZ_SUCCESS); }
  virtual ezStatus InternalCanDisconnect(const ezPin* pSource, const ezPin* pTarget) const { return ezStatus(EZ_SUCCESS); }
  virtual ezStatus InternalCanMoveNode(const ezDocumentObject* pObject, const ezVec2& vPos) const { return ezStatus(EZ_SUCCESS); }
  virtual void InternalCreatePins(const ezDocumentObject* pObject, NodeInternal& node) = 0;
  virtual void InternalDestroyPins(const ezDocumentObject* pObject, NodeInternal& node) = 0;
  virtual ezConnection* InternalCreateConnection(const ezPin* pSource, const ezPin* pTarget) { return EZ_DEFAULT_NEW(ezConnection); }
  virtual void InternalDestroyConnection(ezConnection* pConnection) { EZ_DEFAULT_DELETE(pConnection); }

  void ObjectHandler(const ezDocumentObjectEvent& e);
  void StructureEventHandler(const ezDocumentObjectStructureEvent& e);

private:
  ezMap<ezUuid, NodeInternal> m_ObjectToNode;
  ezMap<PinTuple, ezConnection*> m_PinsToConnection;
};
