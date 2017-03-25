#pragma once

#include <GameEngine/Basics.h>
#include <GameEngine/VisualScript/VisualScriptNode.h>

//////////////////////////////////////////////////////////////////////////

class EZ_GAMEENGINE_DLL ezVisualScriptNode_Add : public ezVisualScriptNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisualScriptNode_Add, ezVisualScriptNode);
public:
  ezVisualScriptNode_Add();

  virtual void Execute(ezVisualScriptInstance* pInstance) override;
  virtual void SetInputPinValue(ezUInt8 uiPin, const ezVariant& value) override;

  double m_Value1 = 0;
  double m_Value2 = 0;
};

//////////////////////////////////////////////////////////////////////////

