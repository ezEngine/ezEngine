#pragma once

#include <PhysXPlugin/PhysXPluginDLL.h>
#include <GameEngine/VisualScript/VisualScriptNode.h>
#include <Core/Messages/TriggerMessage.h>
#include <Foundation/Strings/HashedString.h>
#include <Core/World/Declarations.h>

struct ezMsgPxTriggerTriggered;
class ezVisualScriptInstance;

//////////////////////////////////////////////////////////////////////////

class EZ_PHYSXPLUGIN_DLL ezVisualScriptNode_PxTriggerEvent : public ezVisualScriptNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisualScriptNode_PxTriggerEvent, ezVisualScriptNode);
public:
  ezVisualScriptNode_PxTriggerEvent();
  ~ezVisualScriptNode_PxTriggerEvent();

  virtual void Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin) override;
  virtual void* GetInputPinDataPointer(ezUInt8 uiPin) override { return nullptr; }
  virtual ezInt32 HandlesMessagesWithID() const override;
  virtual void HandleMessage(ezMessage* pMsg) override;

  const char* GetTriggerMessage() const { return m_sTriggerMessage.GetData(); }
  void SetTriggerMessage(const char* s) { m_sTriggerMessage.Assign(s); }

private:
  ezHashedString m_sTriggerMessage;
  ezGameObjectHandle m_hObject;
  ezTriggerState::Enum m_State;
};

//////////////////////////////////////////////////////////////////////////

