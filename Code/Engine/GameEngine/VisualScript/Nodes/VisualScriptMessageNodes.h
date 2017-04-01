#pragma once

#include <GameEngine/Basics.h>
#include <GameEngine/VisualScript/VisualScriptNode.h>
#include <Core/Messages/TriggerMessage.h>

//////////////////////////////////////////////////////////////////////////

class EZ_GAMEENGINE_DLL ezVisualScriptNode_OnUserTriggerMsg : public ezVisualScriptNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisualScriptNode_OnUserTriggerMsg, ezVisualScriptNode);
public:
  ezVisualScriptNode_OnUserTriggerMsg();
  ~ezVisualScriptNode_OnUserTriggerMsg();

  virtual void Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin) override;
  virtual void* GetInputPinDataPointer(ezUInt8 uiPin) override { return nullptr; }
  void OnUserTriggerMsg(ezUserTriggerMessage& msg);

  const char* GetMessage() const { return m_sMessage.GetData(); }
  void SetMessage(const char* s) { m_sMessage = s; }

  ezString m_sMessage;
};

//////////////////////////////////////////////////////////////////////////

class EZ_GAMEENGINE_DLL ezVisualScriptNode_OnTriggerMsg : public ezVisualScriptNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisualScriptNode_OnTriggerMsg, ezVisualScriptNode);
public:
  ezVisualScriptNode_OnTriggerMsg();
  ~ezVisualScriptNode_OnTriggerMsg();

  virtual void Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin) override;
  virtual void* GetInputPinDataPointer(ezUInt8 uiPin) override { return nullptr; }
  void TriggerMessageHandler(ezTriggerMessage& msg);

  const char* GetTriggerMessage() const { return m_sTriggerMessage.GetData(); }
  void SetTriggerMessage(const char* s) { m_sTriggerMessage.Assign(s); }

  ezHashedString m_sTriggerMessage;

private:
  ezGameObjectHandle m_hObject;
};



