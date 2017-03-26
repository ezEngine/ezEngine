#pragma once

#include <GameEngine/Basics.h>
#include <GameEngine/VisualScript/VisualScriptNode.h>

//////////////////////////////////////////////////////////////////////////

class EZ_GAMEENGINE_DLL ezVisualScriptNode_Sequence : public ezVisualScriptNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisualScriptNode_Sequence, ezVisualScriptNode);
public:
  ezVisualScriptNode_Sequence();

  virtual void Execute(ezVisualScriptInstance* pInstance) override;
  virtual void* GetInputPinDataPointer(ezUInt8 uiPin) override { return nullptr; }
};

//////////////////////////////////////////////////////////////////////////

