#pragma once

#include <EditorFramework/Plugin.h>
#include <GuiFoundation/Basics.h>
#include <GuiFoundation/Action/BaseActions.h>

class EZ_EDITORFRAMEWORK_DLL ezGameObjectContextActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapActions(const char* szMapping, const char* szPath);

  static ezActionDescriptorHandle s_hCategory;
  static ezActionDescriptorHandle s_hPickContextScene;
  static ezActionDescriptorHandle s_hPickContextObject;
};

class EZ_EDITORFRAMEWORK_DLL ezGameObjectContextAction : public ezButtonAction
{
  EZ_ADD_DYNAMIC_REFLECTION(ezGameObjectContextAction, ezButtonAction);

public:

  enum class ActionType
  {
    PickContextScene,
    PickContextObject,
  };

  ezGameObjectContextAction(const ezActionContext& context, const char* szName, ActionType type);
  ~ezGameObjectContextAction();

  virtual void Execute(const ezVariant& value) override;

private:
  ActionType m_Type;

};




