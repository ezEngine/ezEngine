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
