#pragma once

#include <GameEngine/Basics.h>
#include <Core/Messages/ScriptFunctionMessage.h>
#include <GameEngine/VisualScript/VisualScriptNode.h>

struct EZ_GAMEENGINE_DLL ezDamageMessage : public ezScriptFunctionMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezDamageMessage, ezScriptFunctionMessage);

  double m_fDamage;
};

class EZ_GAMEENGINE_DLL ezVisualScriptNode_DamageEvent : public ezVisualScriptNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisualScriptNode_DamageEvent, ezVisualScriptNode);
public:
  ezVisualScriptNode_DamageEvent();
  ~ezVisualScriptNode_DamageEvent();

  virtual void Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin) override;
  virtual void* GetInputPinDataPointer(ezUInt8 uiPin) override { return nullptr; }
  virtual ezInt32 HandlesMessagesWithID() const override;
  virtual void HandleMessage(ezMessage* pMsg) override;

private:
  ezDamageMessage m_Msg;
};
