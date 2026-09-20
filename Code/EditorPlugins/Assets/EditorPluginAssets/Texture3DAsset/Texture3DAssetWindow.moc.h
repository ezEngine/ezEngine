#pragma once

#include <EditorEngineProcessFramework/EngineProcess/ViewRenderSettings.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <Foundation/Basics.h>
#include <GuiFoundation/Action/Action.h>
#include <GuiFoundation/Action/BaseActions.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class ezQtOrbitCamViewWidget;
class ezTexture3DAssetDocument;

////////////////////////////////////////////////////////////////////////
// ezTexture3DPreviewModeAction
////////////////////////////////////////////////////////////////////////

class ezTexture3DPreviewModeAction : public ezEnumerationMenuAction
{
  EZ_ADD_DYNAMIC_REFLECTION(ezTexture3DPreviewModeAction, ezEnumerationMenuAction);

public:
  ezTexture3DPreviewModeAction(const ezActionContext& context, const char* szName, const char* szIconPath);
  virtual ezInt64 GetValue() const override;
  virtual void Execute(const ezVariant& value) override;

  /// \brief Fired whenever a document's PreviewMode property is changed through this action, so
  /// that the Slice/Opacity slider actions (registered for the same document) can update their
  /// visibility. Carries the document whose PreviewMode changed.
  static ezEvent<const ezDocument*> s_PreviewModeChangedEvent;

private:
  const ezAbstractMemberProperty* m_pValueProperty = nullptr;
};

//////////////////////////////////////////////////////////////////////////
// ezTexture3DSliceSliderAction
//////////////////////////////////////////////////////////////////////////

class ezTexture3DSliceSliderAction : public ezSliderAction
{
  EZ_ADD_DYNAMIC_REFLECTION(ezTexture3DSliceSliderAction, ezSliderAction);

public:
  ezTexture3DSliceSliderAction(const ezActionContext& context, const char* szName);
  ~ezTexture3DSliceSliderAction();
  virtual void Execute(const ezVariant& value) override;

private:
  void OnPreviewModeChanged(const ezDocument* pDocument);
  void UpdateVisibility();

  const ezAbstractMemberProperty* m_pValueProperty = nullptr;
  ezEventSubscriptionID m_PreviewModeChangedSubscriptionID = 0;
};

//////////////////////////////////////////////////////////////////////////
// ezTexture3DOpacitySliderAction
//////////////////////////////////////////////////////////////////////////

class ezTexture3DOpacitySliderAction : public ezSliderAction
{
  EZ_ADD_DYNAMIC_REFLECTION(ezTexture3DOpacitySliderAction, ezSliderAction);

public:
  ezTexture3DOpacitySliderAction(const ezActionContext& context, const char* szName);
  ~ezTexture3DOpacitySliderAction();
  virtual void Execute(const ezVariant& value) override;

private:
  void OnPreviewModeChanged(const ezDocument* pDocument);
  void UpdateVisibility();

  const ezAbstractMemberProperty* m_pValueProperty = nullptr;
  ezEventSubscriptionID m_PreviewModeChangedSubscriptionID = 0;
};

//////////////////////////////////////////////////////////////////////////
// ezTexture3DAssetActions
//////////////////////////////////////////////////////////////////////////

class ezTexture3DAssetActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapToolbarActions(ezStringView sMapping, bool bShowPreviewModePicker = true);

  static ezActionDescriptorHandle s_hPreviewMode;
  static ezActionDescriptorHandle s_hSliceSlider;
  static ezActionDescriptorHandle s_hOpacitySlider;
};

//////////////////////////////////////////////////////////////////////////
// ezQtTexture3DAssetDocumentWindow
//////////////////////////////////////////////////////////////////////////

class ezQtTexture3DAssetDocumentWindow : public ezQtEngineDocumentWindow
{
  Q_OBJECT

public:
  ezQtTexture3DAssetDocumentWindow(ezTexture3DAssetDocument* pDocument);

private:
  virtual void InternalRedraw() override;
  void SendRedrawMsg();

  ezEngineViewConfig m_ViewConfig;
  ezQtOrbitCamViewWidget* m_pViewWidget;
};
