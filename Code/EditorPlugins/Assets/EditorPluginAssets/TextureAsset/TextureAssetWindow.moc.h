#pragma once

#include <EditorEngineProcessFramework/EngineProcess/ViewRenderSettings.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorPluginAssets/EditorPluginAssetsDLL.h>
#include <GuiFoundation/Action/Action.h>
#include <GuiFoundation/Action/BaseActions.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class ezQtOrbitCamViewWidget;
class ezTextureAssetDocument;

class ezQtTextureAssetDocumentWindow : public ezQtEngineDocumentWindow
{
  Q_OBJECT

public:
  ezQtTextureAssetDocumentWindow(ezTextureAssetDocument* pDocument);

  virtual const char* GetWindowLayoutGroupName() const override { return "TextureAsset"; }

private:
  virtual void InternalRedraw() override;
  void SendRedrawMsg();

  ezEngineViewConfig m_ViewConfig;
  ezQtOrbitCamViewWidget* m_pViewWidget;
};

class EZ_EDITORPLUGINASSETS_DLL ezTextureChannelModeAction : public ezEnumerationMenuAction
{
  EZ_ADD_DYNAMIC_REFLECTION(ezTextureChannelModeAction, ezEnumerationMenuAction);

public:
  ezTextureChannelModeAction(const ezActionContext& context, const char* szName, const char* szIconPath);
  virtual ezInt64 GetValue() const override;
  virtual void Execute(const ezVariant& value) override;

private:
  const ezAbstractMemberProperty* m_pValueProperty = nullptr;
};

class EZ_EDITORPLUGINASSETS_DLL ezTextureLodSliderAction : public ezSliderAction
{
  EZ_ADD_DYNAMIC_REFLECTION(ezTextureLodSliderAction, ezSliderAction);

public:
  ezTextureLodSliderAction(const ezActionContext& context, const char* szName);

  virtual void Execute(const ezVariant& value) override;

private:
  const ezAbstractMemberProperty* m_pValueProperty = nullptr;
};

class ezTextureAssetActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapToolbarActions(ezStringView sMapping);

  static ezActionDescriptorHandle s_hTextureChannelMode;
  static ezActionDescriptorHandle s_hLodSlider;
};
