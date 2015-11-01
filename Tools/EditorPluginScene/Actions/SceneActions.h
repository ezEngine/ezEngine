#pragma once

#include <EditorPluginScene/Plugin.h>
#include <GuiFoundation/Basics.h>
#include <GuiFoundation/Action/BaseActions.h>
#include <EditorPluginScene/Scene/SceneDocument.h>

///
class EZ_EDITORPLUGINSCENE_DLL ezSceneActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapActions(const char* szMapping, const char* szPath);

  static ezActionDescriptorHandle s_hSceneCategory;
  static ezActionDescriptorHandle s_hUpdatePrefabs;
  static ezActionDescriptorHandle s_hExportScene;

};

///
class EZ_EDITORPLUGINSCENE_DLL ezSceneAction : public ezButtonAction
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSceneAction);

public:

  enum class ActionType
  {
    UpdatePrefabs,
    ExportScene,
  };

  ezSceneAction(const ezActionContext& context, const char* szName, ActionType type);
  ~ezSceneAction();

  virtual void Execute(const ezVariant& value) override;

private:
  ezSceneDocument* m_pSceneDocument;
  ActionType m_Type;
};




