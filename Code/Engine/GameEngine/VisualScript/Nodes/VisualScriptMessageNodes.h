#pragma once

#include <GameEngine/Basics.h>
#include <GameEngine/VisualScript/VisualScriptNode.h>
#include <Core/Messages/EventMessage.h>
#include <Core/Messages/TriggerMessage.h>

struct ezMsgInputActionTriggered;

//////////////////////////////////////////////////////////////////////////

class EZ_GAMEENGINE_DLL ezVisualScriptNode_SimpleUserEvent : public ezVisualScriptNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisualScriptNode_SimpleUserEvent, ezVisualScriptNode);
public:
  ezVisualScriptNode_SimpleUserEvent();
  ~ezVisualScriptNode_SimpleUserEvent();

  virtual void Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin) override;
  virtual void* GetInputPinDataPointer(ezUInt8 uiPin) override { return nullptr; }
  virtual ezInt32 HandlesMessagesWithID() const override;
  virtual void HandleMessage(ezMessage* pMsg) override;

  const char* GetMessage() const { return m_sMessage.GetData(); }
  void SetMessage(const char* s) { m_sMessage = s; }

  ezString m_sMessage;
};

//////////////////////////////////////////////////////////////////////////

class EZ_GAMEENGINE_DLL ezVisualScriptNode_GenericEvent : public ezVisualScriptNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisualScriptNode_GenericEvent, ezVisualScriptNode);
public:
  ezVisualScriptNode_GenericEvent();
  ~ezVisualScriptNode_GenericEvent();

  virtual void Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin) override;
  virtual void* GetInputPinDataPointer(ezUInt8 uiPin) override { return nullptr; }
  virtual ezInt32 HandlesMessagesWithID() const override;
  virtual void HandleMessage(ezMessage* pMsg) override;

  const char* GetEventType() const { return m_sEventType.GetData(); }
  void SetEventType(const char* s) { m_sEventType = s; }

  ezString m_sEventType;
};

//////////////////////////////////////////////////////////////////////////

class EZ_GAMEENGINE_DLL ezVisualScriptNode_ScriptUpdateEvent : public ezVisualScriptNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisualScriptNode_ScriptUpdateEvent, ezVisualScriptNode);
public:
  ezVisualScriptNode_ScriptUpdateEvent();
  ~ezVisualScriptNode_ScriptUpdateEvent();

  virtual void Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin) override;
  virtual void* GetInputPinDataPointer(ezUInt8 uiPin) override { return nullptr; }
};

//////////////////////////////////////////////////////////////////////////

class EZ_GAMEENGINE_DLL ezVisualScriptNode_InputState : public ezVisualScriptNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisualScriptNode_InputState, ezVisualScriptNode);
public:
  ezVisualScriptNode_InputState();
  ~ezVisualScriptNode_InputState();

  virtual void Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin) override;
  virtual void* GetInputPinDataPointer(ezUInt8 uiPin) override;

  ezString m_sInputAction;
  bool m_bOnlyKeyPressed = false;
  ezComponentHandle m_hComponent;
};

//////////////////////////////////////////////////////////////////////////

class EZ_GAMEENGINE_DLL ezVisualScriptNode_InputEvent : public ezVisualScriptNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisualScriptNode_InputEvent, ezVisualScriptNode);
public:
  ezVisualScriptNode_InputEvent();
  ~ezVisualScriptNode_InputEvent();

  virtual void Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin) override;
  virtual void* GetInputPinDataPointer(ezUInt8 uiPin) override { return nullptr; }
  virtual ezInt32 HandlesMessagesWithID() const override;
  virtual void HandleMessage(ezMessage* pMsg) override;

  const char* GetInputAction() const { return m_sInputAction.GetData(); }
  void SetInputAction(const char* s) { m_sInputAction.Assign(s); }

private:
  ezHashedString m_sInputAction;
  ezGameObjectHandle m_hSenderObject;
  ezComponentHandle m_hSenderComponent;
  ezTriggerState::Enum m_State;
};

//////////////////////////////////////////////////////////////////////////

