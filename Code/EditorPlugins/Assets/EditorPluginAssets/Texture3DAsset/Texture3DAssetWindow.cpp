#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/DocumentWindow/OrbitCamViewWidget.moc.h>
#include <EditorFramework/InputContexts/EditorInputContext.h>
#include <EditorPluginAssets/Texture3DAsset/Texture3DAsset.h>
#include <EditorPluginAssets/Texture3DAsset/Texture3DAssetWindow.moc.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <GuiFoundation/DockPanels/DocumentPanel.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>

////////////////////////////////////////////////////////////////////////
// ezTexture3DPreviewModeAction
////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTexture3DPreviewModeAction, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezTexture3DPreviewModeAction::ezTexture3DPreviewModeAction(const ezActionContext& context, const char* szName, const char* szIconPath)
  : ezEnumerationMenuAction(context, szName, szIconPath)
{
  auto pDocument = context.m_pDocument;
  m_pValueProperty = ezReflectionUtils::GetMemberProperty(pDocument->GetDynamicRTTI(), "PreviewMode");

  const ezRTTI* pEnumRTTI = m_pValueProperty != nullptr ? m_pValueProperty->GetSpecificType() : ezGetStaticRTTI<ezTexture3DPreviewMode>();
  InitEnumerationType(pEnumRTTI);
}

ezInt64 ezTexture3DPreviewModeAction::GetValue() const
{
  ezVariant value = 0;
  if (m_pValueProperty)
  {
    value = ezReflectionUtils::GetMemberPropertyValue(m_pValueProperty, m_Context.m_pDocument);
  }
  return value.ConvertTo<ezInt64>();
}

void ezTexture3DPreviewModeAction::Execute(const ezVariant& value)
{
  if (m_pValueProperty)
  {
    ezReflectionUtils::SetMemberPropertyValue(m_pValueProperty, m_Context.m_pDocument, value);
    s_PreviewModeChangedEvent.Broadcast(m_Context.m_pDocument);
  }
}

ezEvent<const ezDocument*> ezTexture3DPreviewModeAction::s_PreviewModeChangedEvent;

//////////////////////////////////////////////////////////////////////////
// ezTexture3DSliceSliderAction
//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTexture3DSliceSliderAction, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezTexture3DSliceSliderAction::ezTexture3DSliceSliderAction(const ezActionContext& context, const char* szName)
  : ezSliderAction(context, szName)
{
  auto pDocument = context.m_pDocument;
  m_pValueProperty = ezReflectionUtils::GetMemberProperty(pDocument->GetDynamicRTTI(), "SliceCoordinate");

  ezVariant currentValue = 0.5f;
  if (m_pValueProperty)
  {
    currentValue = ezReflectionUtils::GetMemberPropertyValue(m_pValueProperty, pDocument);
  }

  // 0..1000 maps to a slice coordinate of 0.0 .. 1.0
  SetRange(0, 1000);
  SetValue((int)(currentValue.ConvertTo<float>() * 1000.0f));

  UpdateVisibility();
  m_PreviewModeChangedSubscriptionID = ezTexture3DPreviewModeAction::s_PreviewModeChangedEvent.AddEventHandler(
    ezMakeDelegate(&ezTexture3DSliceSliderAction::OnPreviewModeChanged, this));
}

ezTexture3DSliceSliderAction::~ezTexture3DSliceSliderAction()
{
  ezTexture3DPreviewModeAction::s_PreviewModeChangedEvent.RemoveEventHandler(m_PreviewModeChangedSubscriptionID);
}

void ezTexture3DSliceSliderAction::Execute(const ezVariant& value)
{
  if (m_pValueProperty)
  {
    const float fSliceCoordinate = value.ConvertTo<ezInt32>() / 1000.0f;
    ezReflectionUtils::SetMemberPropertyValue(m_pValueProperty, m_Context.m_pDocument, fSliceCoordinate);
  }
}

void ezTexture3DSliceSliderAction::OnPreviewModeChanged(const ezDocument* pDocument)
{
  if (pDocument == m_Context.m_pDocument)
  {
    UpdateVisibility();
  }
}

void ezTexture3DSliceSliderAction::UpdateVisibility()
{
  ezInt64 iPreviewMode = ezTexture3DPreviewMode::Slices;
  if (const ezAbstractMemberProperty* pPreviewModeProperty = ezReflectionUtils::GetMemberProperty(m_Context.m_pDocument->GetDynamicRTTI(), "PreviewMode"))
  {
    iPreviewMode = ezReflectionUtils::GetMemberPropertyValue(pPreviewModeProperty, m_Context.m_pDocument).ConvertTo<ezInt64>();
  }

  SetVisible(iPreviewMode == ezTexture3DPreviewMode::Slices);
}

//////////////////////////////////////////////////////////////////////////
// ezTexture3DOpacitySliderAction
//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTexture3DOpacitySliderAction, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezTexture3DOpacitySliderAction::ezTexture3DOpacitySliderAction(const ezActionContext& context, const char* szName)
  : ezSliderAction(context, szName)
{
  auto pDocument = context.m_pDocument;
  m_pValueProperty = ezReflectionUtils::GetMemberProperty(pDocument->GetDynamicRTTI(), "OpacityMultiplier");

  ezVariant currentValue = 1.0f;
  if (m_pValueProperty)
  {
    currentValue = ezReflectionUtils::GetMemberPropertyValue(m_pValueProperty, pDocument);
  }

  // 0..2000 maps to an opacity multiplier of 0.0 .. 20.0
  SetRange(0, 2000);
  SetValue((int)(currentValue.ConvertTo<float>() * 100.0f));

  UpdateVisibility();
  m_PreviewModeChangedSubscriptionID = ezTexture3DPreviewModeAction::s_PreviewModeChangedEvent.AddEventHandler(
    ezMakeDelegate(&ezTexture3DOpacitySliderAction::OnPreviewModeChanged, this));
}

ezTexture3DOpacitySliderAction::~ezTexture3DOpacitySliderAction()
{
  ezTexture3DPreviewModeAction::s_PreviewModeChangedEvent.RemoveEventHandler(m_PreviewModeChangedSubscriptionID);
}

void ezTexture3DOpacitySliderAction::Execute(const ezVariant& value)
{
  if (m_pValueProperty)
  {
    const float fOpacityMultiplier = value.ConvertTo<ezInt32>() / 100.0f;
    ezReflectionUtils::SetMemberPropertyValue(m_pValueProperty, m_Context.m_pDocument, fOpacityMultiplier);
  }
}

void ezTexture3DOpacitySliderAction::OnPreviewModeChanged(const ezDocument* pDocument)
{
  if (pDocument == m_Context.m_pDocument)
  {
    UpdateVisibility();
  }
}

void ezTexture3DOpacitySliderAction::UpdateVisibility()
{
  ezInt64 iPreviewMode = ezTexture3DPreviewMode::Slices;
  if (const ezAbstractMemberProperty* pPreviewModeProperty = ezReflectionUtils::GetMemberProperty(m_Context.m_pDocument->GetDynamicRTTI(), "PreviewMode"))
  {
    iPreviewMode = ezReflectionUtils::GetMemberPropertyValue(pPreviewModeProperty, m_Context.m_pDocument).ConvertTo<ezInt64>();
  }

  SetVisible(iPreviewMode == ezTexture3DPreviewMode::RayMarch);
}

//////////////////////////////////////////////////////////////////////////
// ezTexture3DAssetActions
//////////////////////////////////////////////////////////////////////////

ezActionDescriptorHandle ezTexture3DAssetActions::s_hPreviewMode;
ezActionDescriptorHandle ezTexture3DAssetActions::s_hSliceSlider;
ezActionDescriptorHandle ezTexture3DAssetActions::s_hOpacitySlider;

void ezTexture3DAssetActions::RegisterActions()
{
  s_hPreviewMode = EZ_REGISTER_DYNAMIC_MENU("Texture3DAsset.PreviewMode", ezTexture3DPreviewModeAction, ":/EditorFramework/Icons/RenderMode.svg");
  s_hSliceSlider = EZ_REGISTER_ACTION_0("Texture3DAsset.SliceSlider", ezActionScope::Document, "Texture 3D", "", ezTexture3DSliceSliderAction);
  s_hOpacitySlider = EZ_REGISTER_ACTION_0("Texture3DAsset.OpacitySlider", ezActionScope::Document, "Texture 3D", "", ezTexture3DOpacitySliderAction);
}

void ezTexture3DAssetActions::UnregisterActions()
{
  ezActionManager::UnregisterAction(s_hPreviewMode);
  ezActionManager::UnregisterAction(s_hSliceSlider);
  ezActionManager::UnregisterAction(s_hOpacitySlider);
}

void ezTexture3DAssetActions::MapToolbarActions(ezStringView sMapping, bool bShowPreviewModePicker)
{
  ezActionMap* pMap = ezActionMapManager::GetActionMap(sMapping);
  EZ_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", sMapping);

  // LUT assets only ever preview in Slices mode, so the mode picker is omitted for them (see
  // ConfigureLUTAsset() in EditorPluginAssets.cpp) -- ezLUTAssetDocument also forces its
  // PreviewMode to Slices at construction, so the Opacity slider (RayMarch-only) stays hidden too.
  if (bShowPreviewModePicker)
  {
    pMap->MapAction(s_hPreviewMode, "", 14.0f);
  }
  pMap->MapAction(s_hSliceSlider, "", 15.0f);
  pMap->MapAction(s_hOpacitySlider, "", 16.0f);
}

//////////////////////////////////////////////////////////////////////////
// ezQtTexture3DAssetDocumentWindow
//////////////////////////////////////////////////////////////////////////

ezQtTexture3DAssetDocumentWindow::ezQtTexture3DAssetDocumentWindow(ezTexture3DAssetDocument* pDocument)
  : ezQtEngineDocumentWindow(pDocument)
{
  // Menu Bar
  {
    ezQtMenuBarActionMapView* pMenuBar = static_cast<ezQtMenuBarActionMapView*>(menuBar());
    ezActionContext context;
    context.m_sMapping = "Texture3DAssetMenuBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pMenuBar->SetActionContext(context);
  }

  // Tool Bar
  {
    ezQtToolBarActionMapView* pToolBar = new ezQtToolBarActionMapView("Toolbar", this);
    ezActionContext context;
    context.m_sMapping = "Texture3DAssetToolBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pToolBar->SetActionContext(context);
    pToolBar->setObjectName("Texture3DAssetWindowToolBar");
    addToolBar(pToolBar);
  }

  // 3D View
  {
    SetTargetFramerate(25);

    m_ViewConfig.m_Camera.LookAt(ezVec3(-2, 0, 0), ezVec3(0, 0, 0), ezVec3(0, 0, 1));
    m_ViewConfig.ApplyPerspectiveSetting(90);

    m_pViewWidget = new ezQtOrbitCamViewWidget(this, &m_ViewConfig);
    m_pViewWidget->ConfigureFixed(ezVec3(0), ezVec3(0.5f), ezVec3(-2, 0, 0));
    AddViewWidget(m_pViewWidget);
    ezQtViewWidgetContainer* pContainer = new ezQtViewWidgetContainer(GetContainerWindow()->GetDockManager(), this, m_pViewWidget, nullptr);

    m_pDockManager->setCentralWidget(pContainer);
  }

  {
    ezQtDocumentPanel* pPropertyPanel = new ezQtDocumentPanel(GetContainerWindow()->GetDockManager(), this, pDocument);
    pPropertyPanel->setObjectName("Texture3DAssetDockWidget");
    pPropertyPanel->setWindowTitle("Texture 3D Properties");
    pPropertyPanel->show();

    ezQtPropertyGridWidget* pPropertyGrid = new ezQtPropertyGridWidget(pPropertyPanel, pDocument);
    pPropertyPanel->setWidget(pPropertyGrid);

    m_pDockManager->addDockWidgetTab(ads::RightDockWidgetArea, pPropertyPanel);

    pDocument->GetSelectionManager()->SetSelection(pDocument->GetObjectManager()->GetRootObject()->GetChildren()[0]);
  }

  FinishWindowCreation();
}

void ezQtTexture3DAssetDocumentWindow::InternalRedraw()
{
  ezEditorInputContext::UpdateActiveInputContext();
  SendRedrawMsg();
  ezQtEngineDocumentWindow::InternalRedraw();
}

void ezQtTexture3DAssetDocumentWindow::SendRedrawMsg()
{
  // do not try to redraw while the process is crashed, it is obviously futile
  if (ezEditorEngineProcessConnection::GetSingleton()->IsProcessCrashed())
    return;

  {
    const ezTexture3DAssetDocument* pDoc = static_cast<const ezTexture3DAssetDocument*>(GetDocument());

    ezDocumentConfigMsgToEngine msg;
    msg.m_sWhatToDo = "PreviewMode";
    msg.m_iValue = pDoc->m_PreviewMode.GetValue();
    GetEditorEngineConnection()->SendMessage(&msg);

    ezDocumentConfigMsgToEngine msg2;
    msg2.m_sWhatToDo = "SliceCoordinate";
    msg2.m_fValue = pDoc->m_fSliceCoordinate;
    GetEditorEngineConnection()->SendMessage(&msg2);

    ezDocumentConfigMsgToEngine msg3;
    msg3.m_sWhatToDo = "OpacityMultiplier";
    msg3.m_fValue = pDoc->m_fOpacityMultiplier;
    GetEditorEngineConnection()->SendMessage(&msg3);
  }

  for (auto pView : m_ViewWidgets)
  {
    pView->SetEnablePicking(false);
    pView->UpdateCameraInterpolation();
    pView->SyncToEngine();
  }
}
