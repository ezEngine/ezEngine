#include <PCH.h>
#include <GameEngine/VisualScript/Implementation/VisualScriptLogicNodes.h>
#include <GameEngine/VisualScript/VisualScriptInstance.h>

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualScriptNode_Sequence, 1, ezRTTIDefaultAllocator<ezVisualScriptNode_Sequence>)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Logic")
  }
  EZ_END_ATTRIBUTES
  EZ_BEGIN_PROPERTIES
  {
    // Execution Pins (Input)
    EZ_CONSTANT_PROPERTY("execIn", 0)->AddAttributes(new ezVisualScriptInputPinAttribute(ezVisualScriptPinCategory::Execution)),
    // Execution Pins (Output)
    EZ_CONSTANT_PROPERTY("execOut0", 0)->AddAttributes(new ezVisualScriptOutputPinAttribute(ezVisualScriptPinCategory::Execution)),
    EZ_CONSTANT_PROPERTY("execOut1", 1)->AddAttributes(new ezVisualScriptOutputPinAttribute(ezVisualScriptPinCategory::Execution)),
    EZ_CONSTANT_PROPERTY("execOut2", 2)->AddAttributes(new ezVisualScriptOutputPinAttribute(ezVisualScriptPinCategory::Execution)),
    EZ_CONSTANT_PROPERTY("execOut3", 3)->AddAttributes(new ezVisualScriptOutputPinAttribute(ezVisualScriptPinCategory::Execution)),
    EZ_CONSTANT_PROPERTY("execOut4", 4)->AddAttributes(new ezVisualScriptOutputPinAttribute(ezVisualScriptPinCategory::Execution)),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

ezVisualScriptNode_Sequence::ezVisualScriptNode_Sequence() { }

void ezVisualScriptNode_Sequence::Execute(ezVisualScriptInstance* pInstance)
{
  pInstance->ExecuteConnectedNodes(this, 0);
  pInstance->ExecuteConnectedNodes(this, 1);
  pInstance->ExecuteConnectedNodes(this, 2);
  pInstance->ExecuteConnectedNodes(this, 3);
  pInstance->ExecuteConnectedNodes(this, 4);
}

//////////////////////////////////////////////////////////////////////////

