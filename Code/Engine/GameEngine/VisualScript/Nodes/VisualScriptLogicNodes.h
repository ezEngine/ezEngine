#pragma once

#include <GameEngine/GameEngineDLL.h>
#include <GameEngine/VisualScript/VisualScriptNode.h>

//////////////////////////////////////////////////////////////////////////

class EZ_GAMEENGINE_DLL ezVisualScriptNode_Sequence : public ezVisualScriptNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisualScriptNode_Sequence, ezVisualScriptNode);
public:
  ezVisualScriptNode_Sequence();

  virtual void Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin) override;
  virtual void* GetInputPinDataPointer(ezUInt8 uiPin) override { return nullptr; }
};

//////////////////////////////////////////////////////////////////////////

struct EZ_GAMEENGINE_DLL ezLogicOperator
{
  typedef ezUInt8 StorageType;

  enum Enum
  {
    Equal,
    Unequal,
    Less,
    LessEqual,
    Greater,
    GreaterEqual,

    Default = Equal,
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_GAMEENGINE_DLL, ezLogicOperator);

class EZ_GAMEENGINE_DLL ezVisualScriptNode_Compare : public ezVisualScriptNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisualScriptNode_Compare, ezVisualScriptNode);
public:
  ezVisualScriptNode_Compare();
  ~ezVisualScriptNode_Compare();

  virtual void Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin) override;
  virtual void* GetInputPinDataPointer(ezUInt8 uiPin) override;

  ezEnum<ezLogicOperator> m_Operator;

  double m_Value1 = 0.0;
  double m_Value2 = 0.0;
};

//////////////////////////////////////////////////////////////////////////

class EZ_GAMEENGINE_DLL ezVisualScriptNode_CompareExec : public ezVisualScriptNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisualScriptNode_CompareExec, ezVisualScriptNode);
public:
  ezVisualScriptNode_CompareExec();
  ~ezVisualScriptNode_CompareExec();

  virtual void Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin) override;
  virtual void* GetInputPinDataPointer(ezUInt8 uiPin) override;

  ezEnum<ezLogicOperator> m_Operator;

  double m_Value1 = 0.0;
  double m_Value2 = 0.0;
};

//////////////////////////////////////////////////////////////////////////

class EZ_GAMEENGINE_DLL ezVisualScriptNode_If : public ezVisualScriptNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisualScriptNode_If, ezVisualScriptNode);
public:
  ezVisualScriptNode_If();
  ~ezVisualScriptNode_If();

  virtual void Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin) override;
  virtual void* GetInputPinDataPointer(ezUInt8 uiPin) override;

  bool m_Value = false;;
};

//////////////////////////////////////////////////////////////////////////

class EZ_GAMEENGINE_DLL ezVisualScriptNode_Logic : public ezVisualScriptNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisualScriptNode_Logic, ezVisualScriptNode);
public:
  ezVisualScriptNode_Logic();
  ~ezVisualScriptNode_Logic();

  virtual void Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin) override;
  virtual void* GetInputPinDataPointer(ezUInt8 uiPin) override;

  bool m_Value1 = false;
  bool m_Value2 = false;
};

//////////////////////////////////////////////////////////////////////////

