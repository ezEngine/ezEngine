#pragma once

#include <EditorPluginProceduralPlacement/Plugin.h>
#include <GuiFoundation/Basics.h>
#include <GuiFoundation/Action/BaseActions.h>

class ezPreferences;

class EZ_EDITORPLUGINPROCEDURALPLACEMENT_DLL ezProceduralPlacementActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapMenuActions();

  static ezActionDescriptorHandle s_hCategoryProceduralPlacement;
  static ezActionDescriptorHandle s_hDumpAST;
  static ezActionDescriptorHandle s_hDumpDisassembly;
};

class EZ_EDITORPLUGINPROCEDURALPLACEMENT_DLL ezProceduralPlacementAction : public ezButtonAction
{
  EZ_ADD_DYNAMIC_REFLECTION(ezProceduralPlacementAction, ezButtonAction);
public:

  enum class ActionType
  {
    DumpAST,
    DumpDisassembly,
  };

  ezProceduralPlacementAction(const ezActionContext& context, const char* szName, ActionType type);
  ~ezProceduralPlacementAction();

  virtual void Execute(const ezVariant& value) override;

private:
  void OnPreferenceChange(ezPreferences* pref);

  ActionType m_Type;
};
