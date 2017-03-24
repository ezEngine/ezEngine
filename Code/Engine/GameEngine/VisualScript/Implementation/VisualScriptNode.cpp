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

void ezVisualScriptNode::SetOutputPinValue(const ezVisualScriptInstance* pInstance, ezUInt8 uiPin, const ezVariant& value)
{
  const ezUInt32 uiConnectionID = ((ezUInt32)m_uiNodeID << 16) | (ezUInt32)uiPin;

  ezHybridArray<ezUInt32, 2>* TargetNodeAndPins;
  if (!pInstance->m_TargetNodeAndPin.TryGetValue(uiConnectionID, TargetNodeAndPins))
    return;

  for (ezUInt32 uiTargetNodeAndPin : *TargetNodeAndPins)
  {
    const ezUInt32 uiTargetNode = uiTargetNodeAndPin >> 16;
    const ezUInt8 uiTargetPin = uiTargetNodeAndPin & 0xFF;

    pInstance->m_Nodes[uiTargetNode]->SetInputPinValue(uiTargetPin, value);
  }
}

void ezVisualScriptNode::ExecuteTargetNode(const ezVisualScriptInstance* pInstance, ezUInt16 uiNthTarget)
{
  const ezUInt32 uiConnectionID = ((ezUInt32)m_uiNodeID << 16) | (ezUInt32)uiNthTarget;

  ezUInt16 uiTargetNode = 0;
  if (!pInstance->m_TargetNode.TryGetValue(uiConnectionID, uiTargetNode))
    return;

  pInstance->m_Nodes[uiTargetNode]->Execute(pInstance);
}

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualScriptNode_Counter, 1, ezRTTIDefaultAllocator<ezVisualScriptNode_Counter>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_CONSTANT_PROPERTY("execIn",   0)->AddAttributes(new ezVisualScriptInputPinAttribute(ezVisualScriptPinCategory::Execution)),
    EZ_CONSTANT_PROPERTY("execOut0", 0)->AddAttributes(new ezVisualScriptOutputPinAttribute(ezVisualScriptPinCategory::Execution)),
    EZ_CONSTANT_PROPERTY("Count", double(0.0))->AddAttributes(new ezVisualScriptOutputPinAttribute(ezVisualScriptPinCategory::Number)),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

ezVisualScriptNode_Counter::ezVisualScriptNode_Counter()
{
}

void ezVisualScriptNode_Counter::Execute(const ezVisualScriptInstance* pInstance)
{
  ++m_iCounter;
  SetOutputPinValue(pInstance, 0, m_iCounter);

  ExecuteTargetNode(pInstance, 0);
}

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualScriptNode_Printer, 1, ezRTTIDefaultAllocator<ezVisualScriptNode_Printer>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_CONSTANT_PROPERTY("execIn",   0)->AddAttributes(new ezVisualScriptInputPinAttribute(ezVisualScriptPinCategory::Execution)),
    EZ_CONSTANT_PROPERTY("Value", double(0.0))->AddAttributes(new ezVisualScriptInputPinAttribute(ezVisualScriptPinCategory::Number)),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

ezVisualScriptNode_Printer::ezVisualScriptNode_Printer()
{
  m_sPrint = "Current Value: {0}";
}

void ezVisualScriptNode_Printer::Execute(const ezVisualScriptInstance* pInstance)
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
    EZ_CONSTANT_PROPERTY("execIn",   0)->AddAttributes(new ezVisualScriptInputPinAttribute(ezVisualScriptPinCategory::Execution)),
    EZ_CONSTANT_PROPERTY("execOut0",   0)->AddAttributes(new ezVisualScriptOutputPinAttribute(ezVisualScriptPinCategory::Execution)),
    EZ_CONSTANT_PROPERTY("execOut1",   0)->AddAttributes(new ezVisualScriptOutputPinAttribute(ezVisualScriptPinCategory::Execution)),
    EZ_CONSTANT_PROPERTY("Value1", double(0.0))->AddAttributes(new ezVisualScriptInputPinAttribute(ezVisualScriptPinCategory::Number)),
    EZ_CONSTANT_PROPERTY("Value2", double(0.0))->AddAttributes(new ezVisualScriptInputPinAttribute(ezVisualScriptPinCategory::Number)),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

ezVisualScriptNode_If::ezVisualScriptNode_If()
{
  m_Value1 = 0.0;
  m_Value2 = 0.0;
}

void ezVisualScriptNode_If::Execute(const ezVisualScriptInstance* pInstance)
{
  if (m_Value1 < m_Value2)
  {
    ExecuteTargetNode(pInstance, 0);
  }
  else
  {
    ExecuteTargetNode(pInstance, 1);
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
    EZ_CONSTANT_PROPERTY("execOut0", 0)->AddAttributes(new ezVisualScriptOutputPinAttribute(ezVisualScriptPinCategory::Execution)),
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

void ezVisualScriptNode_Input::Execute(const ezVisualScriptInstance* pInstance)
{
  ExecuteTargetNode(pInstance, 0);
}

void ezVisualScriptNode_Input::TriggerMessageHandler(ezTriggerMessage& msg)
{
  if (msg.m_UsageStringHash == m_UsageStringHash)
  {
    m_bStepNode = true;
    ezLog::Info("Trigger Msg arrived");
  }
}

