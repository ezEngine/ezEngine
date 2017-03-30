#pragma once

#include <GameEngine/Basics.h>
#include <GameEngine/VisualScript/VisualScriptNode.h>

//////////////////////////////////////////////////////////////////////////

/// \brief Computes (a1 * a2) + (b1 * b2)
class EZ_GAMEENGINE_DLL ezVisualScriptNode_MultiplyAdd : public ezVisualScriptNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisualScriptNode_MultiplyAdd, ezVisualScriptNode);
public:
  ezVisualScriptNode_MultiplyAdd();
  ~ezVisualScriptNode_MultiplyAdd();

  virtual void Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin) override;
  virtual void* GetInputPinDataPointer(ezUInt8 uiPin) override;

  double m_Value1 = 0;
  double m_Value2 = 1;
  double m_Value3 = 0;
  double m_Value4 = 1;
};

//////////////////////////////////////////////////////////////////////////

/// \brief Computes (a / b)
class EZ_GAMEENGINE_DLL ezVisualScriptNode_Div : public ezVisualScriptNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisualScriptNode_Div, ezVisualScriptNode);
public:
  ezVisualScriptNode_Div();
  ~ezVisualScriptNode_Div();

  virtual void Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin) override;
  virtual void* GetInputPinDataPointer(ezUInt8 uiPin) override;

  double m_Value1 = 1;
  double m_Value2 = 1;
};

//////////////////////////////////////////////////////////////////////////

