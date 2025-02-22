#pragma once

#include <EditorPluginAngelScript/EditorPluginAngelScriptDLL.h>

#include <GuiFoundation/Action/BaseActions.h>
#include <GuiFoundation/GuiFoundationDLL.h>

class ezAngelScriptAssetDocument;
struct ezAngelScriptAssetEvent;

class ezAngelScriptActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapActions(ezStringView sMapping);

  static ezActionDescriptorHandle s_hCategory;
  static ezActionDescriptorHandle s_hOpenInVSC;
  static ezActionDescriptorHandle s_hSyncExposedParams;
};

class ezAngelScriptAction : public ezButtonAction
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAngelScriptAction, ezButtonAction);

public:
  enum class ActionType
  {
    OpenInVSC,
    SyncExposedParameters,
  };

  ezAngelScriptAction(const ezActionContext& context, const char* szName, ActionType type);

  virtual void Execute(const ezVariant& value) override;

private:
  ezAngelScriptAssetDocument* m_pDocument = nullptr;
  ActionType m_Type;
};
