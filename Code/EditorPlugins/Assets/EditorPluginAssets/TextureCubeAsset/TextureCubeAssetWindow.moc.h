#pragma once

#include <EditorEngineProcessFramework/EngineProcess/ViewRenderSettings.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <Foundation/Basics.h>
#include <GuiFoundation/Action/Action.h>
#include <GuiFoundation/Action/BaseActions.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class ezQtOrbitCamViewWidget;
class ezTextureCubeAssetDocument;

class ezQtTextureCubeAssetDocumentWindow : public ezQtEngineDocumentWindow
{
  Q_OBJECT

public:
  ezQtTextureCubeAssetDocumentWindow(ezTextureCubeAssetDocument* pDocument);

  virtual const char* GetWindowLayoutGroupName() const override { return "TextureCubeAsset"; }

private:
  virtual void InternalRedraw() override;
  void SendRedrawMsg();

  ezEngineViewConfig m_ViewConfig;
  ezQtOrbitCamViewWidget* m_pViewWidget;
};

class ezTextureCubeChannelModeAction : public ezEnumerationMenuAction
{
  EZ_ADD_DYNAMIC_REFLECTION(ezTextureCubeChannelModeAction, ezEnumerationMenuAction);

public:
  ezTextureCubeChannelModeAction(const ezActionContext& context, const char* szName, const char* szIconPath);
  virtual ezInt64 GetValue() const override;
  virtual void Execute(const ezVariant& value) override;
};

class ezTextureCubeLodSliderAction : public ezSliderAction
{
  EZ_ADD_DYNAMIC_REFLECTION(ezTextureCubeLodSliderAction, ezSliderAction);

public:
  ezTextureCubeLodSliderAction(const ezActionContext& context, const char* szName);

  virtual void Execute(const ezVariant& value) override;

private:
  ezTextureCubeAssetDocument* m_pDocument;
};

class ezTextureCubeAssetActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapActions(const char* szMapping, const char* szPath);

  static ezActionDescriptorHandle s_hTextureChannelMode;
  static ezActionDescriptorHandle s_hLodSlider;
};

