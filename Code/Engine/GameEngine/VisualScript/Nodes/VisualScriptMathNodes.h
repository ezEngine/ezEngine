#pragma once

#include <GameEngine/GameEngineDLL.h>
#include <GameEngine/VisualScript/VisualScriptNode.h>

//////////////////////////////////////////////////////////////////////////

/// \brief Computes (a1 * a2) + (b1 * b2)
class EZ_GAMEENGINE_DLL ezVisualScriptNode_MultiplyAdd : public ezVisualScriptNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisualScriptNode_MultiplyAdd, ezVisualScriptNode);
public:
  ezVisualScriptNode_MultiplyAdd();
  ~ezVisualScriptNode_MultiplyAdd();

  virtual void Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin) override;
  virtual void* GetInputPinDataPointer(ezUInt8 uiPin) override;

  double m_Value1 = 0;
  double m_Value2 = 1;
  double m_Value3 = 0;
  double m_Value4 = 1;
};

//////////////////////////////////////////////////////////////////////////

/// \brief Computes (a / b)
class EZ_GAMEENGINE_DLL ezVisualScriptNode_Div : public ezVisualScriptNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisualScriptNode_Div, ezVisualScriptNode);
public:
  ezVisualScriptNode_Div();
  ~ezVisualScriptNode_Div();

  virtual void Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin) override;
  virtual void* GetInputPinDataPointer(ezUInt8 uiPin) override;

  double m_Value1 = 1;
  double m_Value2 = 1;
};

//////////////////////////////////////////////////////////////////////////

/// \brief Computes min(a, b)
class EZ_GAMEENGINE_DLL ezVisualScriptNode_Min : public ezVisualScriptNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisualScriptNode_Min, ezVisualScriptNode);
public:
  ezVisualScriptNode_Min();
  ~ezVisualScriptNode_Min();

  virtual void Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin) override;
  virtual void* GetInputPinDataPointer(ezUInt8 uiPin) override;

  double m_Value1 = 0;
  double m_Value2 = 0;
};

//////////////////////////////////////////////////////////////////////////

/// \brief Computes max(a, b)
class EZ_GAMEENGINE_DLL ezVisualScriptNode_Max : public ezVisualScriptNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisualScriptNode_Max, ezVisualScriptNode);
public:
  ezVisualScriptNode_Max();
  ~ezVisualScriptNode_Max();

  virtual void Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin) override;
  virtual void* GetInputPinDataPointer(ezUInt8 uiPin) override;

  double m_Value1 = 0;
  double m_Value2 = 0;
};

//////////////////////////////////////////////////////////////////////////

/// \brief Computes Clamp(Value, MinValue, MaxValue)
class EZ_GAMEENGINE_DLL ezVisualScriptNode_Clamp : public ezVisualScriptNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisualScriptNode_Clamp, ezVisualScriptNode);
public:
  ezVisualScriptNode_Clamp();
  ~ezVisualScriptNode_Clamp();

  virtual void Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin) override;
  virtual void* GetInputPinDataPointer(ezUInt8 uiPin) override;

  double m_Value = 0;
  double m_MinValue = 0;
  double m_MaxValue = 0;
};

//////////////////////////////////////////////////////////////////////////

/// \brief Computes abs(value)
class EZ_GAMEENGINE_DLL ezVisualScriptNode_Abs : public ezVisualScriptNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisualScriptNode_Abs, ezVisualScriptNode);
public:
  ezVisualScriptNode_Abs();
  ~ezVisualScriptNode_Abs();

  virtual void Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin) override;
  virtual void* GetInputPinDataPointer(ezUInt8 uiPin) override;

  double m_Value = 0;
};

//////////////////////////////////////////////////////////////////////////

/// \brief Computes abs(value)
class EZ_GAMEENGINE_DLL ezVisualScriptNode_Sign : public ezVisualScriptNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisualScriptNode_Sign, ezVisualScriptNode);
public:
  ezVisualScriptNode_Sign();
  ~ezVisualScriptNode_Sign();

  virtual void Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin) override;
  virtual void* GetInputPinDataPointer(ezUInt8 uiPin) override;

  double m_Value = 0;
};

//////////////////////////////////////////////////////////////////////////

