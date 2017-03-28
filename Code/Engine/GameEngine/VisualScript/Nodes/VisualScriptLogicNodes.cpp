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
    EZ_CONSTANT_PROPERTY("run", 0)->AddAttributes(new ezVisScriptExecPinInAttribute()),
    // Execution Pins (Output)
    EZ_CONSTANT_PROPERTY("then1", 0)->AddAttributes(new ezVisScriptExecPinOutAttribute(0)),
    EZ_CONSTANT_PROPERTY("then2", 1)->AddAttributes(new ezVisScriptExecPinOutAttribute(1)),
    EZ_CONSTANT_PROPERTY("then3", 2)->AddAttributes(new ezVisScriptExecPinOutAttribute(2)),
    EZ_CONSTANT_PROPERTY("then4", 3)->AddAttributes(new ezVisScriptExecPinOutAttribute(3)),
    EZ_CONSTANT_PROPERTY("then5", 4)->AddAttributes(new ezVisScriptExecPinOutAttribute(4)),
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

