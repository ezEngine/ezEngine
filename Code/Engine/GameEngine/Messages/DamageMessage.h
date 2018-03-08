#pragma once

#include <GameEngine/Basics.h>
#include <GameEngine/VisualScript/VisualScriptNode.h>
#include <Foundation/Communication/Message.h>

struct EZ_GAMEENGINE_DLL ezMsgDamage : public ezMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezMsgDamage, ezMessage);

  double m_fDamage;
};

class EZ_GAMEENGINE_DLL ezVisualScriptNode_OnDamage : public ezVisualScriptNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisualScriptNode_OnDamage, ezVisualScriptNode);
public:
  ezVisualScriptNode_OnDamage();
  ~ezVisualScriptNode_OnDamage();

  virtual void Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin) override;
  virtual void* GetInputPinDataPointer(ezUInt8 uiPin) override { return nullptr; }
  virtual ezInt32 HandlesMessagesWithID() const override;
  virtual void HandleMessage(ezMessage* pMsg) override;

private:
  ezMsgDamage m_Msg;
};
