#pragma once

#include <Foundation/Basics.h>
#include <GameEngine/VisualScript/VisualScriptNode.h>

class EZ_SAMPLEGAMEPLUGIN_DLL ezVisualScriptNode_SampleNode : public ezVisualScriptNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisualScriptNode_SampleNode, ezVisualScriptNode);
public:
  ezVisualScriptNode_SampleNode();

  virtual void Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin) override;
  virtual void* GetInputPinDataPointer(ezUInt8 uiPin) override;

  ezString m_sPrint;
  double m_Value = 0;
};

