#pragma once

#include <GameEngine/Basics.h>
#include <GameEngine/VisualScript/VisualScriptNode.h>
#include <Core/World/Declarations.h>

//////////////////////////////////////////////////////////////////////////

class EZ_GAMEENGINE_DLL ezVisualScriptNode_DeleteObject : public ezVisualScriptNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisualScriptNode_DeleteObject, ezVisualScriptNode);
public:
  ezVisualScriptNode_DeleteObject();

  virtual void Execute(ezVisualScriptInstance* pInstance) override;
  virtual void* GetInputPinDataPointer(ezUInt8 uiPin) override;

private:
  ezGameObjectHandle m_hObject;
};

//////////////////////////////////////////////////////////////////////////

class EZ_GAMEENGINE_DLL ezVisualScriptNode_ActivateComponent : public ezVisualScriptNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisualScriptNode_ActivateComponent, ezVisualScriptNode);
public:
  ezVisualScriptNode_ActivateComponent();

  virtual void Execute(ezVisualScriptInstance* pInstance) override;
  virtual void* GetInputPinDataPointer(ezUInt8 uiPin) override;

private:
  ezComponentHandle m_hComponent;
  bool m_bActive = false;
};

//////////////////////////////////////////////////////////////////////////
