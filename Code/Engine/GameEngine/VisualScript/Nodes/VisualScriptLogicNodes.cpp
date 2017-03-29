#include <PCH.h>
#include <GameEngine/VisualScript/Nodes/VisualScriptLogicNodes.h>
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
    EZ_INPUT_EXECUTION_PIN("run", 0),
    // Execution Pins (Output)
    EZ_OUTPUT_EXECUTION_PIN("then1", 0),
    EZ_OUTPUT_EXECUTION_PIN("then2", 1),
    EZ_OUTPUT_EXECUTION_PIN("then3", 2),
    EZ_OUTPUT_EXECUTION_PIN("then4", 3),
    EZ_OUTPUT_EXECUTION_PIN("then5", 4),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

ezVisualScriptNode_Sequence::ezVisualScriptNode_Sequence() { }

void ezVisualScriptNode_Sequence::Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin)
{
  pInstance->ExecuteConnectedNodes(this, 0);
  pInstance->ExecuteConnectedNodes(this, 1);
  pInstance->ExecuteConnectedNodes(this, 2);
  pInstance->ExecuteConnectedNodes(this, 3);
  pInstance->ExecuteConnectedNodes(this, 4);
}

//////////////////////////////////////////////////////////////////////////

