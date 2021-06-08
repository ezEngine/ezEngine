#pragma once

#include <Core/World/Declarations.h>
#include <GameEngine/GameEngineDLL.h>
#include <GameEngine/VisualScript/VisualScriptNode.h>

class EZ_GAMEENGINE_DLL ezVisualScriptNode_VariableBase : public ezVisualScriptNode
{
protected:
  ezHashedString m_sVariable;

  void SetVariable(const char* szVariable) { m_sVariable.Assign(szVariable); }
  const char* GetVariable() const { return m_sVariable; }
};

//////////////////////////////////////////////////////////////////////////

class EZ_GAMEENGINE_DLL ezVisualScriptNode_GetNumberProperty : public ezVisualScriptNode_VariableBase
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisualScriptNode_GetNumberProperty, ezVisualScriptNode);

public:
  ezVisualScriptNode_GetNumberProperty();
  ~ezVisualScriptNode_GetNumberProperty();

  virtual void Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin) override;
  virtual void* GetInputPinDataPointer(ezUInt8 uiPin) override;

  ezComponentHandle m_hComponent;
};

//////////////////////////////////////////////////////////////////////////

class EZ_GAMEENGINE_DLL ezVisualScriptNode_SetNumberProperty : public ezVisualScriptNode_VariableBase
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisualScriptNode_SetNumberProperty, ezVisualScriptNode);

public:
  ezVisualScriptNode_SetNumberProperty();
  ~ezVisualScriptNode_SetNumberProperty();

  virtual void Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin) override;
  virtual void* GetInputPinDataPointer(ezUInt8 uiPin) override;

  ezComponentHandle m_hComponent;
  double m_fValue = 0;
};

//////////////////////////////////////////////////////////////////////////

class EZ_GAMEENGINE_DLL ezVisualScriptNode_GetBoolProperty : public ezVisualScriptNode_VariableBase
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisualScriptNode_GetBoolProperty, ezVisualScriptNode);

public:
  ezVisualScriptNode_GetBoolProperty();
  ~ezVisualScriptNode_GetBoolProperty();

  virtual void Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin) override;
  virtual void* GetInputPinDataPointer(ezUInt8 uiPin) override;

  ezComponentHandle m_hComponent;
};

//////////////////////////////////////////////////////////////////////////

class EZ_GAMEENGINE_DLL ezVisualScriptNode_SetBoolProperty : public ezVisualScriptNode_VariableBase
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisualScriptNode_SetBoolProperty, ezVisualScriptNode);

public:
  ezVisualScriptNode_SetBoolProperty();
  ~ezVisualScriptNode_SetBoolProperty();

  virtual void Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin) override;
  virtual void* GetInputPinDataPointer(ezUInt8 uiPin) override;

  ezComponentHandle m_hComponent;
  bool m_bValue = false;
};

//////////////////////////////////////////////////////////////////////////

class EZ_GAMEENGINE_DLL ezVisualScriptNode_GetStringProperty : public ezVisualScriptNode_VariableBase
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisualScriptNode_GetStringProperty, ezVisualScriptNode);

public:
  ezVisualScriptNode_GetStringProperty();
  ~ezVisualScriptNode_GetStringProperty();

  virtual void Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin) override;
  virtual void* GetInputPinDataPointer(ezUInt8 uiPin) override;

  ezComponentHandle m_hComponent;
};

//////////////////////////////////////////////////////////////////////////

class EZ_GAMEENGINE_DLL ezVisualScriptNode_SetStringProperty : public ezVisualScriptNode_VariableBase
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisualScriptNode_SetStringProperty, ezVisualScriptNode);

public:
  ezVisualScriptNode_SetStringProperty();
  ~ezVisualScriptNode_SetStringProperty();

  virtual void Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin) override;
  virtual void* GetInputPinDataPointer(ezUInt8 uiPin) override;

  ezComponentHandle m_hComponent;
  ezString m_sValue;
};

//////////////////////////////////////////////////////////////////////////

class EZ_GAMEENGINE_DLL ezVisualScriptNode_Number : public ezVisualScriptNode_VariableBase
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisualScriptNode_Number, ezVisualScriptNode);

public:
  ezVisualScriptNode_Number();
  ~ezVisualScriptNode_Number();

  virtual void Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin) override;
  virtual void* GetInputPinDataPointer(ezUInt8 uiPin) override { return nullptr; }
};

//////////////////////////////////////////////////////////////////////////

class EZ_GAMEENGINE_DLL ezVisualScriptNode_StoreNumber : public ezVisualScriptNode_VariableBase
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisualScriptNode_StoreNumber, ezVisualScriptNode);

public:
  ezVisualScriptNode_StoreNumber();
  ~ezVisualScriptNode_StoreNumber();

  virtual void Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin) override;
  virtual void* GetInputPinDataPointer(ezUInt8 uiPin) override;

  double m_Value = 0;
};

//////////////////////////////////////////////////////////////////////////

class EZ_GAMEENGINE_DLL ezVisualScriptNode_Bool : public ezVisualScriptNode_VariableBase
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisualScriptNode_Bool, ezVisualScriptNode);

public:
  ezVisualScriptNode_Bool();
  ~ezVisualScriptNode_Bool();

  virtual void Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin) override;
  virtual void* GetInputPinDataPointer(ezUInt8 uiPin) override { return nullptr; }
};

//////////////////////////////////////////////////////////////////////////

class EZ_GAMEENGINE_DLL ezVisualScriptNode_StoreBool : public ezVisualScriptNode_VariableBase
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisualScriptNode_StoreBool, ezVisualScriptNode);

public:
  ezVisualScriptNode_StoreBool();
  ~ezVisualScriptNode_StoreBool();

  virtual void Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin) override;
  virtual void* GetInputPinDataPointer(ezUInt8 uiPin) override;

  bool m_Value = false;
};

//////////////////////////////////////////////////////////////////////////

class EZ_GAMEENGINE_DLL ezVisualScriptNode_ToggleBool : public ezVisualScriptNode_VariableBase
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisualScriptNode_ToggleBool, ezVisualScriptNode);

public:
  ezVisualScriptNode_ToggleBool();
  ~ezVisualScriptNode_ToggleBool();

  virtual void Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin) override;
  virtual void* GetInputPinDataPointer(ezUInt8 uiPin) override { return nullptr; }
};

//////////////////////////////////////////////////////////////////////////

class EZ_GAMEENGINE_DLL ezVisualScriptNode_String : public ezVisualScriptNode_VariableBase
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisualScriptNode_String, ezVisualScriptNode);

public:
  ezVisualScriptNode_String();
  ~ezVisualScriptNode_String();

  virtual void Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin) override;
  virtual void* GetInputPinDataPointer(ezUInt8 uiPin) override { return nullptr; }
};

//////////////////////////////////////////////////////////////////////////

class EZ_GAMEENGINE_DLL ezVisualScriptNode_StoreString : public ezVisualScriptNode_VariableBase
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisualScriptNode_StoreString, ezVisualScriptNode);

public:
  ezVisualScriptNode_StoreString();
  ~ezVisualScriptNode_StoreString();

  virtual void Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin) override;
  virtual void* GetInputPinDataPointer(ezUInt8 uiPin) override;

  ezString m_sValue;
};

//////////////////////////////////////////////////////////////////////////
