#pragma once

#include <Core/Messages/EventMessage.h>
#include <Foundation/Communication/Message.h>
#include <GameEngine/GameEngineDLL.h>
#include <GameEngine/VisualScript/VisualScriptNode.h>

struct EZ_GAMEENGINE_DLL ezMsgDamage : public ezEventMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezMsgDamage, ezEventMessage);

  double m_fDamage = 0;
  ezString m_sHitObjectName; ///< The actual game object that was hit (may be a child of the object to which the message is sent)
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
