#pragma once

#include <EditorEngineProcessFramework/EngineProcess/ViewRenderSettings.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <GuiFoundation/Action/BaseActions.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class ezQtOrbitCamViewWidget;
class ezSubstancePackageAssetDocument;

class ezQtSubstancePackageAssetWindow : public ezQtEngineDocumentWindow
{
  Q_OBJECT

public:
  ezQtSubstancePackageAssetWindow(ezSubstancePackageAssetDocument* pDocument);

  virtual const char* GetWindowLayoutGroupName() const override { return "SubstancePackageAsset"; }

private:
  virtual void InternalRedraw() override;
  void SendRedrawMsg();

  ezEngineViewConfig m_ViewConfig;
  ezQtOrbitCamViewWidget* m_pViewWidget;
};

//////////////////////////////////////////////////////////////////////////

class ezSubstanceSelectOutputAction : public ezDynamicMenuAction
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSubstanceSelectOutputAction, ezDynamicMenuAction);

public:
  ezSubstanceSelectOutputAction(const ezActionContext& context, const char* szName, const char* szIconPath);

  virtual void GetEntries(ezDynamicArray<Item>& out_entries) override;
  virtual void Execute(const ezVariant& value) override;
};

//////////////////////////////////////////////////////////////////////////

class ezSubstancePackageAssetActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapToolbarActions(ezStringView sMapping);

  static ezActionDescriptorHandle s_hSelectedOutput;
  static ezActionDescriptorHandle s_hTextureChannelMode;
  static ezActionDescriptorHandle s_hLodSlider;
};
