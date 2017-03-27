#pragma once

#include <GameEngine/Basics.h>
#include <GameEngine/VisualScript/VisualScriptNode.h>
#include <Core/World/Declarations.h>

//////////////////////////////////////////////////////////////////////////

class EZ_GAMEENGINE_DLL ezVisualScriptNode_FindChildObject : public ezVisualScriptNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisualScriptNode_FindChildObject, ezVisualScriptNode);
public:
  ezVisualScriptNode_FindChildObject();

  virtual void Execute(ezVisualScriptInstance* pInstance) override;
  virtual void* GetInputPinDataPointer(ezUInt8 uiPin) override { return nullptr; }

private:
  ezGameObjectHandle m_hObject;
  ezString m_sObjectName;
};

//////////////////////////////////////////////////////////////////////////

class EZ_GAMEENGINE_DLL ezVisualScriptNode_FindComponent : public ezVisualScriptNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisualScriptNode_FindComponent, ezVisualScriptNode);
public:
  ezVisualScriptNode_FindComponent();

  virtual void Execute(ezVisualScriptInstance* pInstance) override;
  virtual void* GetInputPinDataPointer(ezUInt8 uiPin) override;

private:
  ezGameObjectHandle m_hObject;
  ezComponentHandle m_hComponent;
  ezString m_sType;
};

//////////////////////////////////////////////////////////////////////////
