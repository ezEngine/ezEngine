#pragma once

#include <GameEngine/Basics.h>
#include <GameEngine/VisualScript/VisualScriptNode.h>
#include <Core/World/Declarations.h>

//////////////////////////////////////////////////////////////////////////

class EZ_GAMEENGINE_DLL ezVisualScriptNode_GetNumberProperty : public ezVisualScriptNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisualScriptNode_GetNumberProperty, ezVisualScriptNode);
public:
  ezVisualScriptNode_GetNumberProperty();
  ~ezVisualScriptNode_GetNumberProperty();

  virtual void Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin) override;
  virtual void* GetInputPinDataPointer(ezUInt8 uiPin) override;

  ezComponentHandle m_hComponent;
  ezString m_sVariable;
};

//////////////////////////////////////////////////////////////////////////

class EZ_GAMEENGINE_DLL ezVisualScriptNode_SetNumberProperty : public ezVisualScriptNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisualScriptNode_SetNumberProperty, ezVisualScriptNode);
public:
  ezVisualScriptNode_SetNumberProperty();
  ~ezVisualScriptNode_SetNumberProperty();

  virtual void Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin) override;
  virtual void* GetInputPinDataPointer(ezUInt8 uiPin) override;

  ezComponentHandle m_hComponent;
  ezString m_sVariable;
  double m_fValue = 0;
};

//////////////////////////////////////////////////////////////////////////

class EZ_GAMEENGINE_DLL ezVisualScriptNode_GetBoolProperty : public ezVisualScriptNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisualScriptNode_GetBoolProperty, ezVisualScriptNode);
public:
  ezVisualScriptNode_GetBoolProperty();
  ~ezVisualScriptNode_GetBoolProperty();

  virtual void Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin) override;
  virtual void* GetInputPinDataPointer(ezUInt8 uiPin) override;

  ezComponentHandle m_hComponent;
  ezString m_sVariable;
};

//////////////////////////////////////////////////////////////////////////

class EZ_GAMEENGINE_DLL ezVisualScriptNode_SetBoolProperty : public ezVisualScriptNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisualScriptNode_SetBoolProperty, ezVisualScriptNode);
public:
  ezVisualScriptNode_SetBoolProperty();
  ~ezVisualScriptNode_SetBoolProperty();

  virtual void Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin) override;
  virtual void* GetInputPinDataPointer(ezUInt8 uiPin) override;

  ezComponentHandle m_hComponent;
  ezString m_sVariable;
  bool m_bValue = false;
};

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
  double m_Value = 0;
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
  double m_Value = 0;
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
  bool m_Value = false;
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
  bool m_Value = false;
};

//////////////////////////////////////////////////////////////////////////

class EZ_GAMEENGINE_DLL ezVisualScriptNode_ToggleBool : public ezVisualScriptNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisualScriptNode_ToggleBool, ezVisualScriptNode);
public:
  ezVisualScriptNode_ToggleBool();
  ~ezVisualScriptNode_ToggleBool();

  virtual void Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin) override;
  virtual void* GetInputPinDataPointer(ezUInt8 uiPin) override { return nullptr; }

  ezString m_sVariable;
  ezTempHashedString m_VarName;
  bool m_Value = false;
};

//////////////////////////////////////////////////////////////////////////

