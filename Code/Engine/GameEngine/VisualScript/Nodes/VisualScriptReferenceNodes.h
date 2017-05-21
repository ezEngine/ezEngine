#pragma once

#include <GameEngine/Basics.h>
#include <GameEngine/VisualScript/VisualScriptNode.h>
#include <Core/World/Declarations.h>

//////////////////////////////////////////////////////////////////////////

class EZ_GAMEENGINE_DLL ezVisualScriptNode_GetScriptOwner : public ezVisualScriptNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisualScriptNode_GetScriptOwner, ezVisualScriptNode);
public:
  ezVisualScriptNode_GetScriptOwner();
  ~ezVisualScriptNode_GetScriptOwner();

  virtual void Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin) override;
  virtual void* GetInputPinDataPointer(ezUInt8 uiPin) override { return nullptr; }
};

//////////////////////////////////////////////////////////////////////////

class EZ_GAMEENGINE_DLL ezVisualScriptNode_GetComponentOwner : public ezVisualScriptNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisualScriptNode_GetComponentOwner, ezVisualScriptNode);
public:
  ezVisualScriptNode_GetComponentOwner();
  ~ezVisualScriptNode_GetComponentOwner();

  virtual void Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin) override;
  virtual void* GetInputPinDataPointer(ezUInt8 uiPin) override;

private:
  ezComponentHandle m_hComponent;
};

//////////////////////////////////////////////////////////////////////////

class EZ_GAMEENGINE_DLL ezVisualScriptNode_FindChildObject : public ezVisualScriptNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisualScriptNode_FindChildObject, ezVisualScriptNode);
public:
  ezVisualScriptNode_FindChildObject();
  ~ezVisualScriptNode_FindChildObject();

  virtual void Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin) override;
  virtual void* GetInputPinDataPointer(ezUInt8 uiPin) override;

private:
  ezGameObjectHandle m_hObject;
  ezString m_sChildObjectName;
};

//////////////////////////////////////////////////////////////////////////

class EZ_GAMEENGINE_DLL ezVisualScriptNode_FindComponent : public ezVisualScriptNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisualScriptNode_FindComponent, ezVisualScriptNode);
public:
  ezVisualScriptNode_FindComponent();
  ~ezVisualScriptNode_FindComponent();

  virtual void Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin) override;
  virtual void* GetInputPinDataPointer(ezUInt8 uiPin) override;

private:
  ezGameObjectHandle m_hObject;
  ezString m_sType;
};

//////////////////////////////////////////////////////////////////////////

class EZ_GAMEENGINE_DLL ezVisualScriptNode_QueryGlobalObject : public ezVisualScriptNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisualScriptNode_QueryGlobalObject, ezVisualScriptNode);
public:
  ezVisualScriptNode_QueryGlobalObject();
  ~ezVisualScriptNode_QueryGlobalObject();

  virtual void Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin) override;
  virtual void* GetInputPinDataPointer(ezUInt8 uiPin) override { return nullptr; }

private:
  ezString m_sObjectName;
};

//////////////////////////////////////////////////////////////////////////

class EZ_GAMEENGINE_DLL ezVisualScriptNode_FindParent : public ezVisualScriptNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisualScriptNode_FindParent, ezVisualScriptNode);
public:
  ezVisualScriptNode_FindParent();
  ~ezVisualScriptNode_FindParent();

  virtual void Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin) override;
  virtual void* GetInputPinDataPointer(ezUInt8 uiPin) override;

private:
  ezGameObjectHandle m_hObject;
  ezString m_sObjectName;
};

//////////////////////////////////////////////////////////////////////////



