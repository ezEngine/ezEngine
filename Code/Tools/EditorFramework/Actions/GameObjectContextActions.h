#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/Action/BaseActions.h>

class EZ_EDITORFRAMEWORK_DLL ezGameObjectContextActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapToolbarActions(const char* szMapping, const char* szPath);
  static void MapContextMenuActions(const char* szMapping, const char* szPath);

  static ezActionDescriptorHandle s_hCategory;
  static ezActionDescriptorHandle s_hPickContextScene;
  static ezActionDescriptorHandle s_hPickContextObject;
  static ezActionDescriptorHandle s_hClearContextObject;
};

class EZ_EDITORFRAMEWORK_DLL ezGameObjectContextAction : public ezButtonAction
{
  EZ_ADD_DYNAMIC_REFLECTION(ezGameObjectContextAction, ezButtonAction);

public:

  enum class ActionType
  {
    PickContextScene,
    PickContextObject,
    ClearContextObject,
  };

  ezGameObjectContextAction(const ezActionContext& context, const char* szName, ActionType type);
  ~ezGameObjectContextAction();

  virtual void Execute(const ezVariant& value) override;

private:
  void SelectionEventHandler(const ezSelectionManagerEvent& e);
  void Update();

  ActionType m_Type;

};




