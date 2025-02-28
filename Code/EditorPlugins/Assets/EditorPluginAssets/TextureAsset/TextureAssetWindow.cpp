#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/Assets/AssetStatusIndicator.moc.h>
#include <EditorFramework/DocumentWindow/OrbitCamViewWidget.moc.h>
#include <EditorFramework/InputContexts/EditorInputContext.h>
#include <EditorPluginAssets/TextureAsset/TextureAsset.h>
#include <EditorPluginAssets/TextureAsset/TextureAssetWindow.moc.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <GuiFoundation/DockPanels/DocumentPanel.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>

////////////////////////////////////////////////////////////////////////
// ezTextureChannelModeAction
////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTextureChannelModeAction, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezTextureChannelModeAction::ezTextureChannelModeAction(const ezActionContext& context, const char* szName, const char* szIconPath)
  : ezEnumerationMenuAction(context, szName, szIconPath)
{
  auto pDocument = context.m_pDocument;
  m_pValueProperty = ezReflectionUtils::GetMemberProperty(pDocument->GetDynamicRTTI(), "ChannelMode");

  const ezRTTI* pEnumRTTI = m_pValueProperty != nullptr ? m_pValueProperty->GetSpecificType() : ezGetStaticRTTI<ezTextureChannelMode>();
  InitEnumerationType(pEnumRTTI);
}

ezInt64 ezTextureChannelModeAction::GetValue() const
{
  ezVariant value = 0;
  if (m_pValueProperty)
  {
    value = ezReflectionUtils::GetMemberPropertyValue(m_pValueProperty, m_Context.m_pDocument);
  }
  return value.ConvertTo<ezInt64>();
}

void ezTextureChannelModeAction::Execute(const ezVariant& value)
{
  if (m_pValueProperty)
  {
    ezReflectionUtils::SetMemberPropertyValue(m_pValueProperty, m_Context.m_pDocument, value);
  }
}

//////////////////////////////////////////////////////////////////////////
// ezTextureLodSliderAction
//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTextureLodSliderAction, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;


ezTextureLodSliderAction::ezTextureLodSliderAction(const ezActionContext& context, const char* szName)
  : ezSliderAction(context, szName)
{
  auto pDocument = context.m_pDocument;
  m_pValueProperty = ezReflectionUtils::GetMemberProperty(pDocument->GetDynamicRTTI(), "TextureLod");

  ezVariant currentValue = -1;
  if (m_pValueProperty)
  {
    currentValue = ezReflectionUtils::GetMemberPropertyValue(m_pValueProperty, pDocument);
  }

  SetRange(-1, 13);
  SetValue(currentValue.ConvertTo<int>());
}

void ezTextureLodSliderAction::Execute(const ezVariant& value)
{
  if (m_pValueProperty)
  {
    ezReflectionUtils::SetMemberPropertyValue(m_pValueProperty, m_Context.m_pDocument, value);
  }
}


//////////////////////////////////////////////////////////////////////////
// ezTextureAssetActions
//////////////////////////////////////////////////////////////////////////

ezActionDescriptorHandle ezTextureAssetActions::s_hTextureChannelMode;
ezActionDescriptorHandle ezTextureAssetActions::s_hLodSlider;

void ezTextureAssetActions::RegisterActions()
{
  s_hTextureChannelMode = EZ_REGISTER_DYNAMIC_MENU("TextureAsset.ChannelMode", ezTextureChannelModeAction, ":/EditorFramework/Icons/RenderMode.svg");
  s_hLodSlider = EZ_REGISTER_ACTION_0("TextureAsset.LodSlider", ezActionScope::Document, "Texture 2D", "", ezTextureLodSliderAction);
}

void ezTextureAssetActions::UnregisterActions()
{
  ezActionManager::UnregisterAction(s_hTextureChannelMode);
  ezActionManager::UnregisterAction(s_hLodSlider);
}

void ezTextureAssetActions::MapToolbarActions(ezStringView sMapping)
{
  ezActionMap* pMap = ezActionMapManager::GetActionMap(sMapping);
  EZ_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", sMapping);

  pMap->MapAction(s_hLodSlider, "", 14.0f);
  pMap->MapAction(s_hTextureChannelMode, "", 15.0f);
}


//////////////////////////////////////////////////////////////////////////
// ezQtTextureAssetDocumentWindow
//////////////////////////////////////////////////////////////////////////

ezQtTextureAssetDocumentWindow::ezQtTextureAssetDocumentWindow(ezTextureAssetDocument* pDocument)
  : ezQtEngineDocumentWindow(pDocument)
{
  // Menu Bar
  {
    ezQtMenuBarActionMapView* pMenuBar = static_cast<ezQtMenuBarActionMapView*>(menuBar());
    ezActionContext context;
    context.m_sMapping = "TextureAssetMenuBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pMenuBar->SetActionContext(context);
  }

  // Tool Bar
  {
    ezQtToolBarActionMapView* pToolBar = new ezQtToolBarActionMapView("Toolbar", this);
    ezActionContext context;
    context.m_sMapping = "TextureAssetToolBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pToolBar->SetActionContext(context);
    pToolBar->setObjectName("TextureAssetWindowToolBar");
    addToolBar(pToolBar);
  }

  // 3D View
  {
    SetTargetFramerate(25);

    m_ViewConfig.m_Camera.LookAt(ezVec3(-2, 0, 0), ezVec3(0, 0, 0), ezVec3(0, 0, 1));
    m_ViewConfig.ApplyPerspectiveSetting(90);

    m_pViewWidget = new ezQtOrbitCamViewWidget(this, &m_ViewConfig);
    m_pViewWidget->ConfigureFixed(ezVec3(0), ezVec3(0.0f), ezVec3(-1, 0, 0));
    AddViewWidget(m_pViewWidget);
    ezQtViewWidgetContainer* pContainer = new ezQtViewWidgetContainer(GetContainerWindow()->GetDockManager(), this, m_pViewWidget, nullptr);

    m_pDockManager->setCentralWidget(pContainer);
  }

  {
    ezQtDocumentPanel* pPropertyPanel = new ezQtDocumentPanel(GetContainerWindow()->GetDockManager(), this, pDocument);
    pPropertyPanel->setObjectName("TextureAssetDockWidget");
    pPropertyPanel->setWindowTitle("Texture Properties");
    pPropertyPanel->show();

    ezQtPropertyGridWidget* pPropertyGrid = new ezQtPropertyGridWidget(pPropertyPanel, pDocument);

    QWidget* pWidget = new QWidget();
    pWidget->setObjectName("Group");
    pWidget->setLayout(new QVBoxLayout());
    pWidget->setContentsMargins(0, 0, 0, 0);

    pWidget->layout()->setContentsMargins(0, 0, 0, 0);
    pWidget->layout()->addWidget(new ezQtAssetStatusIndicator(GetDocument()));
    pWidget->layout()->addWidget(pPropertyGrid);

    pPropertyPanel->setWidget(pWidget, ads::CDockWidget::ForceNoScrollArea);

    m_pDockManager->addDockWidgetTab(ads::RightDockWidgetArea, pPropertyPanel);

    pDocument->GetSelectionManager()->SetSelection(pDocument->GetObjectManager()->GetRootObject()->GetChildren()[0]);
  }

  FinishWindowCreation();
}

void ezQtTextureAssetDocumentWindow::InternalRedraw()
{
  ezEditorInputContext::UpdateActiveInputContext();
  SendRedrawMsg();
  ezQtEngineDocumentWindow::InternalRedraw();
}

void ezQtTextureAssetDocumentWindow::SendRedrawMsg()
{
  // do not try to redraw while the process is crashed, it is obviously futile
  if (ezEditorEngineProcessConnection::GetSingleton()->IsProcessCrashed())
    return;

  {
    const ezTextureAssetDocument* pDoc = static_cast<const ezTextureAssetDocument*>(GetDocument());
    const ezTextureAssetProperties* pProps = pDoc->GetProperties();

    {
      ezDocumentConfigMsgToEngine msg;
      msg.m_sWhatToDo = "SetChannelMode";
      msg.m_iValue = pDoc->m_ChannelMode.GetValue();
      msg.m_fValue = pProps->m_fAlphaThreshold;
      GetEditorEngineConnection()->SendMessage(&msg);
    }

    {
      ezDocumentConfigMsgToEngine msg;
      msg.m_sWhatToDo = "SetLodLevel";
      msg.m_iValue = pDoc->m_iTextureLod;
      GetEditorEngineConnection()->SendMessage(&msg);
    }
  }

  for (auto pView : m_ViewWidgets)
  {
    pView->SetEnablePicking(false);
    pView->UpdateCameraInterpolation();
    pView->SyncToEngine();
  }
}
