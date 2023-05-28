#pragma once

#include <Core/Messages/EventMessage.h>
#include <Core/Messages/TriggerMessage.h>
#include <GameEngine/GameEngineDLL.h>
#include <GameEngine/VisualScript/VisualScriptNode.h>



//////////////////////////////////////////////////////////////////////////

class EZ_GAMEENGINE_DLL ezVisualScriptNode_ScriptStartEvent : public ezVisualScriptNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisualScriptNode_ScriptStartEvent, ezVisualScriptNode);

public:
  ezVisualScriptNode_ScriptStartEvent();
  ~ezVisualScriptNode_ScriptStartEvent();

  virtual void Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin) override;
  virtual void* GetInputPinDataPointer(ezUInt8 uiPin) override { return nullptr; }
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

  const char* GetMessage() const { return m_sMessage; }
  void SetMessage(const char* s) { m_sMessage.Assign(s); }

private:
  ezHashedString m_sMessage;
  ezVariant m_Value;
};

//////////////////////////////////////////////////////////////////////////

class EZ_GAMEENGINE_DLL ezVisualScriptNode_PhysicsTriggerEvent : public ezVisualScriptNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisualScriptNode_PhysicsTriggerEvent, ezVisualScriptNode);

public:
  ezVisualScriptNode_PhysicsTriggerEvent();
  ~ezVisualScriptNode_PhysicsTriggerEvent();

  virtual void Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin) override;
  virtual void* GetInputPinDataPointer(ezUInt8 uiPin) override { return nullptr; }
  virtual ezInt32 HandlesMessagesWithID() const override;
  virtual void HandleMessage(ezMessage* pMsg) override;

  const char* GetTriggerMessage() const { return m_sTriggerMessage.GetData(); }
  void SetTriggerMessage(const char* s) { m_sTriggerMessage.Assign(s); }

private:
  ezHashedString m_sTriggerMessage;
  ezGameObjectHandle m_hObject;
  ezTriggerState::Enum m_State = ezTriggerState::Enum::Default;
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
  ezTriggerState::Enum m_State = ezTriggerState::Default;
};

//////////////////////////////////////////////////////////////////////////
