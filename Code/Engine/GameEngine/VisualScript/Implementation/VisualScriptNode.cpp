#include <PCH.h>
#include <GameEngine/VisualScript/VisualScriptNode.h>
#include <GameEngine/VisualScript/VisualScriptInstance.h>
#include <Core/Messages/TriggerMessage.h>
#include <Core/Messages/CollisionMessage.h>

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualScriptInputPinAttribute, 1, ezRTTIDefaultAllocator<ezVisualScriptInputPinAttribute>)
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualScriptOutputPinAttribute, 1, ezRTTIDefaultAllocator<ezVisualScriptOutputPinAttribute>)
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
    if (const ezVisualScriptInputPinAttribute* pAttr = prop->GetAttributeByType<ezVisualScriptInputPinAttribute>())
    {
      if (pAttr->GetCategory() == ezVisualScriptPinCategory::Execution)
        return true;
    }

    if (const ezVisualScriptOutputPinAttribute* pAttr = prop->GetAttributeByType<ezVisualScriptOutputPinAttribute>())
    {
      if (pAttr->GetCategory() == ezVisualScriptPinCategory::Execution)
        return true;
    }
  }

  return false;
}

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualScriptNode_Counter, 1, ezRTTIDefaultAllocator<ezVisualScriptNode_Counter>)
{
  EZ_BEGIN_PROPERTIES
  {
    // Execution Pins
    EZ_CONSTANT_PROPERTY("execIn",   0)->AddAttributes(new ezVisualScriptInputPinAttribute(ezVisualScriptPinCategory::Execution)),
    EZ_CONSTANT_PROPERTY("execOut0", 0)->AddAttributes(new ezVisualScriptOutputPinAttribute(ezVisualScriptPinCategory::Execution)),
    // Data Pins
    EZ_CONSTANT_PROPERTY("Count", 0)->AddAttributes(new ezVisualScriptOutputPinAttribute(ezVisualScriptPinCategory::Number)),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

ezVisualScriptNode_Counter::ezVisualScriptNode_Counter()
{
}

void ezVisualScriptNode_Counter::Execute(ezVisualScriptInstance* pInstance)
{
  ++m_iCounter;
  pInstance->SetOutputPinValue(this, 0, m_iCounter);

  pInstance->ExecuteConnectedNodes(this, 0);
}

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualScriptNode_Printer, 1, ezRTTIDefaultAllocator<ezVisualScriptNode_Printer>)
{
  EZ_BEGIN_PROPERTIES
  {
    // Execution Pins
    EZ_CONSTANT_PROPERTY("execIn",   0)->AddAttributes(new ezVisualScriptInputPinAttribute(ezVisualScriptPinCategory::Execution)),
    // Data Pins
    EZ_CONSTANT_PROPERTY("Value", 0)->AddAttributes(new ezVisualScriptInputPinAttribute(ezVisualScriptPinCategory::Number)),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

ezVisualScriptNode_Printer::ezVisualScriptNode_Printer()
{
  m_sPrint = "Current Value: {0}";
}

void ezVisualScriptNode_Printer::Execute(ezVisualScriptInstance* pInstance)
{
  ezLog::Info(m_sPrint, m_iValue);
}

void ezVisualScriptNode_Printer::SetInputPinValue(ezUInt8 uiPin, const ezVariant& value)
{
  m_iValue = value.ConvertTo<ezInt32>();
}

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualScriptNode_If, 1, ezRTTIDefaultAllocator<ezVisualScriptNode_If>)
{
  EZ_BEGIN_PROPERTIES
  {
    // Execution Pins
    EZ_CONSTANT_PROPERTY("execIn",   0)->AddAttributes(new ezVisualScriptInputPinAttribute(ezVisualScriptPinCategory::Execution)),
    EZ_CONSTANT_PROPERTY("execOut0", 0)->AddAttributes(new ezVisualScriptOutputPinAttribute(ezVisualScriptPinCategory::Execution)),
    EZ_CONSTANT_PROPERTY("execOut1", 1)->AddAttributes(new ezVisualScriptOutputPinAttribute(ezVisualScriptPinCategory::Execution)),
    // Data Pins
    EZ_CONSTANT_PROPERTY("Value1", 0)->AddAttributes(new ezVisualScriptInputPinAttribute(ezVisualScriptPinCategory::Number)),
    EZ_CONSTANT_PROPERTY("Value2", 1)->AddAttributes(new ezVisualScriptInputPinAttribute(ezVisualScriptPinCategory::Number)),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

ezVisualScriptNode_If::ezVisualScriptNode_If()
{
  m_Value1 = 0.0;
  m_Value2 = 0.0;
}

void ezVisualScriptNode_If::Execute(ezVisualScriptInstance* pInstance)
{
  if (m_Value1 < m_Value2)
  {
    pInstance->ExecuteConnectedNodes(this, 0);
  }
  else
  {
    pInstance->ExecuteConnectedNodes(this, 1);
  }
}

void ezVisualScriptNode_If::SetInputPinValue(ezUInt8 uiPin, const ezVariant& value)
{
  if (!value.CanConvertTo<double>())
    return;

  switch (uiPin)
  {
  case 0:
    m_Value1 = value.ConvertTo<double>();
    return;

  case 1:
    m_Value2 = value.ConvertTo<double>();
    return;
  }
}


//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualScriptNode_Input, 1, ezRTTIDefaultAllocator<ezVisualScriptNode_Input>)
{
  EZ_BEGIN_PROPERTIES
  {
    // Execution Pins
    EZ_CONSTANT_PROPERTY("execOut0", 0)->AddAttributes(new ezVisualScriptOutputPinAttribute(ezVisualScriptPinCategory::Execution)),
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

void ezVisualScriptNode_Input::Execute(ezVisualScriptInstance* pInstance)
{
  pInstance->ExecuteConnectedNodes(this, 0);
}

void ezVisualScriptNode_Input::TriggerMessageHandler(ezTriggerMessage& msg)
{
  //if (msg.m_UsageStringHash == m_UsageStringHash)
  {
    m_bStepNode = true;
    ezLog::Info("Trigger Msg arrived");
  }
}

