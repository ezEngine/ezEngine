#pragma once

#include <EditorPluginDLang/EditorPluginDLangDLL.h>

#include <GuiFoundation/Action/BaseActions.h>
#include <GuiFoundation/GuiFoundationDLL.h>

class ezDLangAssetDocument;
struct ezDLangAssetEvent;

class ezDLangActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapActions(const char* szMapping, const char* szPath);

  static ezActionDescriptorHandle s_hCategory;
  static ezActionDescriptorHandle s_hEditScript;
};

class ezDLangAction : public ezButtonAction
{
  EZ_ADD_DYNAMIC_REFLECTION(ezDLangAction, ezButtonAction);

public:
  enum class ActionType
  {
    EditScript,
  };

  ezDLangAction(const ezActionContext& context, const char* szName, ActionType type, float fSimSpeed = 1.0f);

  virtual void Execute(const ezVariant& value) override;

private:
  ezDLangAssetDocument* m_pDocument = nullptr;
  ActionType m_Type;
};
