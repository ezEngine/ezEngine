#include <SampleGamePlugin/SampleGamePluginPCH.h>

#include <Foundation/Logging/Log.h>
#include <GameEngine/VisualScript/VisualScriptInstance.h>
#include <SampleGamePlugin/Script/VisualScriptNodes.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualScriptNode_SampleNode, 1, ezRTTIDefaultAllocator<ezVisualScriptNode_SampleNode>)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Sample"),
    new ezTitleAttribute("Sample Node '{Value}'"),
  }
    EZ_END_ATTRIBUTES;
    EZ_BEGIN_PROPERTIES
  {
    // Execution Pins
    EZ_INPUT_EXECUTION_PIN("run", 0),
    EZ_OUTPUT_EXECUTION_PIN("then", 0),
    // Data Pins
    EZ_INPUT_DATA_PIN_AND_PROPERTY("Value", 0, ezVisualScriptDataPinType::Number, m_Value),
    EZ_OUTPUT_DATA_PIN("NewValue", 0, ezVisualScriptDataPinType::Number),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezVisualScriptNode_SampleNode::ezVisualScriptNode_SampleNode()
{
  m_sPrint = "Value: {0}";
}

void ezVisualScriptNode_SampleNode::Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin)
{
  if (m_bInputValuesChanged)
  {
    // do stuff that depends on input values
    // skipping pInstance->SetOutputPinValue is valid, if the result cannot have changed (and saves some performance)
  }

  ezLog::Info(m_sPrint, m_Value);
  m_Value += 1;

  pInstance->SetOutputPinValue(this, 0, &m_Value);
  pInstance->ExecuteConnectedNodes(this, 0);
}

void* ezVisualScriptNode_SampleNode::GetInputPinDataPointer(ezUInt8 uiPin)
{
  switch (uiPin)
  {
    case 0:
      return &m_Value;
  }

  return nullptr;
}
