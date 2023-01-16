#include <GameEngine/GameEnginePCH.h>

#include <GameEngine/VisualScript/Nodes/VisualScriptVariantNodes.h>
#include <GameEngine/VisualScript/VisualScriptInstance.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualScriptNode_ConvertTo, 1, ezRTTIDefaultAllocator<ezVisualScriptNode_ConvertTo>)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Variant"),
  }
  EZ_END_ATTRIBUTES;
  EZ_BEGIN_PROPERTIES
  {
    // Data Pins (Input)
    EZ_MEMBER_PROPERTY("Value", m_Value)->AddAttributes(new ezVisScriptDataPinInAttribute(0, ezVisualScriptDataPinType::Variant), new ezDefaultValueAttribute(0)),
    // Data Pins (Output)
    EZ_OUTPUT_DATA_PIN("Bool", 0, ezVisualScriptDataPinType::Boolean),
    EZ_OUTPUT_DATA_PIN("Number", 1, ezVisualScriptDataPinType::Number),
    EZ_OUTPUT_DATA_PIN("String", 2, ezVisualScriptDataPinType::String),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezVisualScriptNode_ConvertTo::ezVisualScriptNode_ConvertTo() = default;
ezVisualScriptNode_ConvertTo::~ezVisualScriptNode_ConvertTo() = default;

void ezVisualScriptNode_ConvertTo::Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin)
{
  if (m_bInputValuesChanged)
  {
    bool bResult = m_Value.ConvertTo<bool>();
    pInstance->SetOutputPinValue(this, 0, &bResult);

    double fResult = m_Value.ConvertTo<double>();
    pInstance->SetOutputPinValue(this, 1, &fResult);

    ezString sResult = m_Value.ConvertTo<ezString>();
    pInstance->SetOutputPinValue(this, 2, &sResult);
  }
}

void* ezVisualScriptNode_ConvertTo::GetInputPinDataPointer(ezUInt8 uiPin)
{
  switch (uiPin)
  {
    case 0:
      return &m_Value;
  }

  return nullptr;
}


EZ_STATICLINK_FILE(GameEngine, GameEngine_VisualScript_Nodes_VisualScriptVariantNodes);

