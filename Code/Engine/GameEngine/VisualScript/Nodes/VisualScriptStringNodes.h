#pragma once

#include <GameEngine/GameEngineDLL.h>
#include <GameEngine/VisualScript/VisualScriptNode.h>

class EZ_GAMEENGINE_DLL ezVisualScriptNode_IsEqual : public ezVisualScriptNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisualScriptNode_IsEqual, ezVisualScriptNode);

public:
  ezVisualScriptNode_IsEqual();
  ~ezVisualScriptNode_IsEqual();

  virtual void Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin) override;
  virtual void* GetInputPinDataPointer(ezUInt8 uiPin) override;

  ezString m_sValue1;
  ezString m_sValue2;

  bool m_bIgnoreCase = false;
};

//////////////////////////////////////////////////////////////////////////

class EZ_GAMEENGINE_DLL ezVisualScriptNode_Format : public ezVisualScriptNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisualScriptNode_Format, ezVisualScriptNode);

public:
  ezVisualScriptNode_Format();
  ~ezVisualScriptNode_Format();

  virtual void Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin) override;
  virtual void* GetInputPinDataPointer(ezUInt8 uiPin) override;

  ezString m_sFormat;
  ezVariant m_Value0 = 0;
  ezVariant m_Value1 = 0;
  ezVariant m_Value2 = 0;
};
