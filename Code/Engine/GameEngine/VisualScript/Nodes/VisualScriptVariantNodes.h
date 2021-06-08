#pragma once

#include <GameEngine/GameEngineDLL.h>
#include <GameEngine/VisualScript/VisualScriptNode.h>

class EZ_GAMEENGINE_DLL ezVisualScriptNode_ConvertTo : public ezVisualScriptNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisualScriptNode_ConvertTo, ezVisualScriptNode);

public:
  ezVisualScriptNode_ConvertTo();
  ~ezVisualScriptNode_ConvertTo();

  virtual void Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin) override;
  virtual void* GetInputPinDataPointer(ezUInt8 uiPin) override;

  ezVariant m_Value = 0;
};
