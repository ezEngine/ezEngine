#pragma once

#include <GameEngine/Basics.h>
#include <GameEngine/VisualScript/VisualScriptNode.h>

//////////////////////////////////////////////////////////////////////////

class EZ_GAMEENGINE_DLL ezVisualScriptNode_Number : public ezVisualScriptNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisualScriptNode_Number, ezVisualScriptNode);
public:
  ezVisualScriptNode_Number();
  ~ezVisualScriptNode_Number();

  virtual void Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin) override;
  virtual void* GetInputPinDataPointer(ezUInt8 uiPin) override { return nullptr; }

  ezString m_sVariable;
  ezTempHashedString m_VarName;
  double m_Value;
};

//////////////////////////////////////////////////////////////////////////

class EZ_GAMEENGINE_DLL ezVisualScriptNode_StoreNumber : public ezVisualScriptNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisualScriptNode_StoreNumber, ezVisualScriptNode);
public:
  ezVisualScriptNode_StoreNumber();
  ~ezVisualScriptNode_StoreNumber();

  virtual void Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin) override;
  virtual void* GetInputPinDataPointer(ezUInt8 uiPin) override;

  ezString m_sVariable;
  ezTempHashedString m_VarName;
  double m_Value;
};

//////////////////////////////////////////////////////////////////////////

class EZ_GAMEENGINE_DLL ezVisualScriptNode_Bool : public ezVisualScriptNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisualScriptNode_Bool, ezVisualScriptNode);
public:
  ezVisualScriptNode_Bool();
  ~ezVisualScriptNode_Bool();

  virtual void Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin) override;
  virtual void* GetInputPinDataPointer(ezUInt8 uiPin) override { return nullptr; }

  ezString m_sVariable;
  ezTempHashedString m_VarName;
  bool m_Value;
};

//////////////////////////////////////////////////////////////////////////

class EZ_GAMEENGINE_DLL ezVisualScriptNode_StoreBool : public ezVisualScriptNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisualScriptNode_StoreBool, ezVisualScriptNode);
public:
  ezVisualScriptNode_StoreBool();
  ~ezVisualScriptNode_StoreBool();

  virtual void Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin) override;
  virtual void* GetInputPinDataPointer(ezUInt8 uiPin) override;

  ezString m_sVariable;
  ezTempHashedString m_VarName;
  bool m_Value;
};

//////////////////////////////////////////////////////////////////////////

