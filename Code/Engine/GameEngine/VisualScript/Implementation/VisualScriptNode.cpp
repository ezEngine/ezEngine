#include <PCH.h>
#include <GameEngine/VisualScript/VisualScriptNode.h>
#include <GameEngine/VisualScript/VisualScriptInstance.h>
#include <Core/Messages/TriggerMessage.h>
#include <Core/Messages/CollisionMessage.h>

//////////////////////////////////////////////////////////////////////////

/// \todo To synchronize attributes across processes, the members must be reflected. Use dummy string attributes to store/restore ezRTTI.

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisScriptExecPinOutAttribute, 1, ezRTTIDefaultAllocator<ezVisScriptExecPinOutAttribute>)
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisScriptExecPinInAttribute, 1, ezRTTIDefaultAllocator<ezVisScriptExecPinInAttribute>)
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisScriptDataPinInAttribute, 1, ezRTTIDefaultAllocator<ezVisScriptDataPinInAttribute>)
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisScriptDataPinOutAttribute, 1, ezRTTIDefaultAllocator<ezVisScriptDataPinOutAttribute>)
EZ_END_DYNAMIC_REFLECTED_TYPE

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualScriptNode, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE

ezVisualScriptNode::ezVisualScriptNode()
{
}

bool ezVisualScriptNode::IsManuallyStepped() const
{
  ezHybridArray<ezAbstractProperty*, 32> properties;
  GetDynamicRTTI()->GetAllProperties(properties);

  for (auto prop : properties)
  {
    if (const ezVisScriptExecPinOutAttribute* pAttr = prop->GetAttributeByType<ezVisScriptExecPinOutAttribute>())
      return true;

    if (const ezVisScriptExecPinInAttribute* pAttr = prop->GetAttributeByType<ezVisScriptExecPinInAttribute>())
      return true;
  }

  return false;
}

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualScriptNode_Counter, 1, ezRTTIDefaultAllocator<ezVisualScriptNode_Counter>)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Test")
  }
    EZ_END_ATTRIBUTES
    EZ_BEGIN_PROPERTIES
  {
    // Execution Pins
    EZ_INPUT_EXECUTION_PIN("run", 0),
    EZ_OUTPUT_EXECUTION_PIN("then", 0),
    // Data Pins
    EZ_MEMBER_PROPERTY("StartValue", m_Counter),
    EZ_OUTPUT_DATA_PIN("Count", 0, ezVisualScriptDataPinType::Number),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

ezVisualScriptNode_Counter::ezVisualScriptNode_Counter() {}

void ezVisualScriptNode_Counter::Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin)
{
  m_Counter += 1;
  pInstance->SetOutputPinValue(this, 0, &m_Counter);

  pInstance->ExecuteConnectedNodes(this, 0);
}

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualScriptNode_Printer, 1, ezRTTIDefaultAllocator<ezVisualScriptNode_Printer>)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Debug")
  }
    EZ_END_ATTRIBUTES
    EZ_BEGIN_PROPERTIES
  {
    // Execution Pins
    EZ_INPUT_EXECUTION_PIN("run", 0),
    EZ_OUTPUT_EXECUTION_PIN("then", 0),
    // Data Pins
    EZ_INPUT_DATA_PIN_AND_PROPERTY("Value", 0, ezVisualScriptDataPinType::Number, m_Value),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

ezVisualScriptNode_Printer::ezVisualScriptNode_Printer()
{
  m_sPrint = "Current Value: {0}";
}

void ezVisualScriptNode_Printer::Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin)
{
  ezLog::Info(m_sPrint, m_Value);

  pInstance->ExecuteConnectedNodes(this, 0);
}

void* ezVisualScriptNode_Printer::GetInputPinDataPointer(ezUInt8 uiPin)
{
  switch (uiPin)
  {
  case 0:
    return &m_Value;
  }

  return nullptr;
}

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualScriptNode_Compare, 1, ezRTTIDefaultAllocator<ezVisualScriptNode_Compare>)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Logic")
  }
    EZ_END_ATTRIBUTES
    EZ_BEGIN_PROPERTIES
  {
    // Execution Pins
    EZ_INPUT_EXECUTION_PIN("run", 0),
    EZ_OUTPUT_EXECUTION_PIN("OnTrue", 0),
    EZ_OUTPUT_EXECUTION_PIN("OnFalse", 1),
    // Data Pins (Input)
    EZ_INPUT_DATA_PIN_AND_PROPERTY("Value1", 0, ezVisualScriptDataPinType::Number, m_Value1),
    EZ_INPUT_DATA_PIN_AND_PROPERTY("Value2", 1, ezVisualScriptDataPinType::Number, m_Value2),
    // Data Pins (Output)
    EZ_OUTPUT_DATA_PIN("Result", 0, ezVisualScriptDataPinType::Boolean),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

ezVisualScriptNode_Compare::ezVisualScriptNode_Compare()
{
  m_Value1 = 0.0;
  m_Value2 = 0.0;
}

void ezVisualScriptNode_Compare::Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin)
{
  if (m_Value1 < m_Value2)
  {
    bool result = true;
    pInstance->SetOutputPinValue(this, 0, &result);
    pInstance->ExecuteConnectedNodes(this, 0);
  }
  else
  {
    bool result = false;
    pInstance->SetOutputPinValue(this, 0, &result);
    pInstance->ExecuteConnectedNodes(this, 1);
  }
}

void* ezVisualScriptNode_Compare::GetInputPinDataPointer(ezUInt8 uiPin)
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

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualScriptNode_Input, 1, ezRTTIDefaultAllocator<ezVisualScriptNode_Input>)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Input")
  }
  EZ_END_ATTRIBUTES
  EZ_BEGIN_PROPERTIES
  {
    //Properties
    EZ_ACCESSOR_PROPERTY("Trigger", GetTrigger, SetTrigger),
    // Execution Pins
    EZ_OUTPUT_EXECUTION_PIN("OnMsg", 0),
    // Data Pins
  }
  EZ_END_PROPERTIES

    EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezTriggerMessage, TriggerMessageHandler),
  }
  EZ_END_MESSAGEHANDLERS
}
EZ_END_DYNAMIC_REFLECTED_TYPE

ezVisualScriptNode_Input::ezVisualScriptNode_Input()
{
}

void ezVisualScriptNode_Input::Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin)
{
  pInstance->ExecuteConnectedNodes(this, 0);
}

void ezVisualScriptNode_Input::TriggerMessageHandler(ezTriggerMessage& msg)
{
  if (msg.m_UsageStringHash == m_sTrigger.GetHash())
  {
    m_bStepNode = true;
    ezLog::Info("Trigger Msg '{0}' arrived", m_sTrigger.GetData());
  }
}

