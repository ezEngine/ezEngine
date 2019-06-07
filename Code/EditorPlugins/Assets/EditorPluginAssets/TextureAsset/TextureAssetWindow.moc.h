#pragma once

#include <EditorEngineProcessFramework/EngineProcess/ViewRenderSettings.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <Foundation/Basics.h>
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

  virtual const char* GetWindowLayoutGroupName() const { return "TextureAsset"; }

private:
  virtual void InternalRedraw() override;
  void SendRedrawMsg();

  ezEngineViewConfig m_ViewConfig;
  ezQtOrbitCamViewWidget* m_pViewWidget;
};

class ezTextureChannelModeAction : public ezEnumerationMenuAction
{
  EZ_ADD_DYNAMIC_REFLECTION(ezTextureChannelModeAction, ezEnumerationMenuAction);

public:
  ezTextureChannelModeAction(const ezActionContext& context, const char* szName, const char* szIconPath);
  virtual ezInt64 GetValue() const override;
  virtual void Execute(const ezVariant& value) override;
};

class ezTextureLodSliderAction : public ezSliderAction
{
  EZ_ADD_DYNAMIC_REFLECTION(ezTextureLodSliderAction, ezSliderAction);

public:
  ezTextureLodSliderAction(const ezActionContext& context, const char* szName);

  virtual void Execute(const ezVariant& value) override;

private:
  ezTextureAssetDocument* m_pDocument;
};

class ezTextureAssetActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapActions(const char* szMapping, const char* szPath);

  static ezActionDescriptorHandle s_hTextureChannelMode;
  static ezActionDescriptorHandle s_hLodSlider;
};
