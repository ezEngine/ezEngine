#pragma once

#include <EditorPluginScene/EditorPluginSceneDLL.h>
#include <GuiFoundation/Action/BaseActions.h>
#include <GuiFoundation/GuiFoundationDLL.h>

class ezScene2Document;
struct ezScene2LayerEvent;

  ///
class EZ_EDITORPLUGINSCENE_DLL ezLayerActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapContextMenuActions(const char* szMapping, const char* szPath);

  static ezActionDescriptorHandle s_hLayerCategory;
  static ezActionDescriptorHandle s_hCreateLayer;
  static ezActionDescriptorHandle s_hDeleteLayer;
  static ezActionDescriptorHandle s_hLayerLoaded;
  static ezActionDescriptorHandle s_hLayerVisible;
};

///
class EZ_EDITORPLUGINSCENE_DLL ezLayerAction : public ezButtonAction
{
  EZ_ADD_DYNAMIC_REFLECTION(ezLayerAction, ezButtonAction);

public:
  enum class ActionType
  {
    CreateLayer,
    DeleteLayer,
    LayerLoaded,
    LayerVisible,
  };

  ezLayerAction(const ezActionContext& context, const char* szName, ActionType type);
  ~ezLayerAction();

  virtual void Execute(const ezVariant& value) override;

private:
  void LayerEventHandler(const ezScene2LayerEvent& e);
	void UpdateEnableState();
  ezUuid GetCurrentSelectedLayer() const;

private:
  ezScene2Document* m_pSceneDocument;
  ActionType m_Type;
};
