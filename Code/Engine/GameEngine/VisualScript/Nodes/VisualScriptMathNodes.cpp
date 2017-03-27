#include <PCH.h>
#include <GameEngine/VisualScript/Nodes/VisualScriptMathNodes.h>
#include <GameEngine/VisualScript/VisualScriptInstance.h>

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualScriptNode_Add, 1, ezRTTIDefaultAllocator<ezVisualScriptNode_Add>)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Math")
  }
  EZ_END_ATTRIBUTES
  EZ_BEGIN_PROPERTIES
  {
    // Data Pins (Input)
    EZ_MEMBER_PROPERTY("Value1", m_Value1)->AddAttributes(new ezVisScriptDataPinInAttribute(0, ezVisualScriptDataPinType::Number)),
    EZ_MEMBER_PROPERTY("Value2", m_Value2)->AddAttributes(new ezVisScriptDataPinInAttribute(1, ezVisualScriptDataPinType::Number)),
    // Data Pins (Output)
    EZ_CONSTANT_PROPERTY("Sum", 0)->AddAttributes(new ezVisScriptDataPinOutAttribute(0, ezVisualScriptDataPinType::Number)),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

ezVisualScriptNode_Add::ezVisualScriptNode_Add() { }

void ezVisualScriptNode_Add::Execute(ezVisualScriptInstance* pInstance)
{
  const double result = m_Value1 + m_Value2;
  pInstance->SetOutputPinValue(this, 0, &result);
}

void* ezVisualScriptNode_Add::GetInputPinDataPointer(ezUInt8 uiPin)
{
  switch (uiPin)
  {
  case 0:
    return &m_Value1;
  case 1:
    return &m_Value2;
  }

  return nullptr;
}

//////////////////////////////////////////////////////////////////////////

