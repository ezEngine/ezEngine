#pragma once

#include <EditorPluginProcGen/EditorPluginProcGenDLL.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/Action/BaseActions.h>

class ezPreferences;

class EZ_EDITORPLUGINPROCGEN_DLL ezProcGenActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapMenuActions();

  static ezActionDescriptorHandle s_hCategory;
  static ezActionDescriptorHandle s_hDumpAST;
  static ezActionDescriptorHandle s_hDumpDisassembly;
};

class EZ_EDITORPLUGINPROCGEN_DLL ezProcGenAction : public ezButtonAction
{
  EZ_ADD_DYNAMIC_REFLECTION(ezProcGenAction, ezButtonAction);
public:

  enum class ActionType
  {
    DumpAST,
    DumpDisassembly,
  };

  ezProcGenAction(const ezActionContext& context, const char* szName, ActionType type);
  ~ezProcGenAction();

  virtual void Execute(const ezVariant& value) override;

private:
  void OnPreferenceChange(ezPreferences* pref);

  ActionType m_Type;
};
