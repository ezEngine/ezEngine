#include <PCH.h>

#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/DocumentWindow/OrbitCamViewWidget.moc.h>
#include <EditorFramework/InputContexts/EditorInputContext.h>
#include <EditorPluginAssets/TextureCubeAsset/TextureCubeAsset.h>
#include <EditorPluginAssets/TextureCubeAsset/TextureCubeAssetObjects.h>
#include <EditorPluginAssets/TextureCubeAsset/TextureCubeAssetWindow.moc.h>
#include <Texture/Image/ImageConversion.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <GuiFoundation/DockPanels/DocumentPanel.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <GuiFoundation/Widgets/ImageWidget.moc.h>

////////////////////////////////////////////////////////////////////////
// ezTextureCubeChannelModeAction
////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTextureCubeChannelModeAction, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezTextureCubeChannelModeAction::ezTextureCubeChannelModeAction(const ezActionContext& context, const char* szName, const char* szIconPath)
    : ezEnumerationMenuAction(context, szName, szIconPath)
{
  InitEnumerationType(ezGetStaticRTTI<ezTextureCubeChannelMode>());
}

ezInt64 ezTextureCubeChannelModeAction::GetValue() const
{
  return static_cast<const ezTextureCubeAssetDocument*>(m_Context.m_pDocument)->m_ChannelMode.GetValue();
}

void ezTextureCubeChannelModeAction::Execute(const ezVariant& value)
{
  ((ezTextureCubeAssetDocument*)m_Context.m_pDocument)->m_ChannelMode.SetValue(value.ConvertTo<ezInt32>());
}

//////////////////////////////////////////////////////////////////////////
// ezTextureCubeLodSliderAction
//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTextureCubeLodSliderAction, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE;


ezTextureCubeLodSliderAction::ezTextureCubeLodSliderAction(const ezActionContext& context, const char* szName)
    : ezSliderAction(context, szName)
{
  m_pDocument = const_cast<ezTextureCubeAssetDocument*>(static_cast<const ezTextureCubeAssetDocument*>(context.m_pDocument));

  SetRange(-1, 13);
  SetValue(m_pDocument->m_iTextureLod);
}

void ezTextureCubeLodSliderAction::Execute(const ezVariant& value)
{
  const ezInt32 iValue = value.Get<ezInt32>();

  m_pDocument->m_iTextureLod = value.Get<ezInt32>();
}


//////////////////////////////////////////////////////////////////////////
// ezTextureCubeAssetActions
//////////////////////////////////////////////////////////////////////////

ezActionDescriptorHandle ezTextureCubeAssetActions::s_hTextureChannelMode;
ezActionDescriptorHandle ezTextureCubeAssetActions::s_hLodSlider;

void ezTextureCubeAssetActions::RegisterActions()
{
  s_hTextureChannelMode =
      EZ_REGISTER_DYNAMIC_MENU("TextureCubeAsset.ChannelMode", ezTextureCubeChannelModeAction, ":/EditorFramework/Icons/RenderMode.png");
  s_hLodSlider =
      EZ_REGISTER_ACTION_0("TextureCubeAsset.LodSlider", ezActionScope::Document, "TextureCube Asset", "", ezTextureCubeLodSliderAction);
}

void ezTextureCubeAssetActions::UnregisterActions()
{
  ezActionManager::UnregisterAction(s_hTextureChannelMode);
  ezActionManager::UnregisterAction(s_hLodSlider);
}

void ezTextureCubeAssetActions::MapActions(const char* szMapping, const char* szPath)
{
  ezActionMap* pMap = ezActionMapManager::GetActionMap(szMapping);
  EZ_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", szMapping);

  pMap->MapAction(s_hLodSlider, szPath, 14.0f);
  pMap->MapAction(s_hTextureChannelMode, szPath, 15.0f);
}


//////////////////////////////////////////////////////////////////////////
// ezQtTextureCubeAssetDocumentWindow
//////////////////////////////////////////////////////////////////////////

ezQtTextureCubeAssetDocumentWindow::ezQtTextureCubeAssetDocumentWindow(ezTextureCubeAssetDocument* pDocument)
    : ezQtEngineDocumentWindow(pDocument)
{
  // Menu Bar
  {
    ezQtMenuBarActionMapView* pMenuBar = static_cast<ezQtMenuBarActionMapView*>(menuBar());
    ezActionContext context;
    context.m_sMapping = "TextureCubeAssetMenuBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pMenuBar->SetActionContext(context);
  }

  // Tool Bar
  {
    ezQtToolBarActionMapView* pToolBar = new ezQtToolBarActionMapView("Toolbar", this);
    ezActionContext context;
    context.m_sMapping = "TextureCubeAssetToolBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pToolBar->SetActionContext(context);
    pToolBar->setObjectName("TextureCubeAssetWindowToolBar");
    addToolBar(pToolBar);
  }

  // 3D View
  {
    SetTargetFramerate(25);

    m_ViewConfig.m_Camera.LookAt(ezVec3(-2, 0, 0), ezVec3(0, 0, 0), ezVec3(0, 0, 1));
    m_ViewConfig.ApplyPerspectiveSetting(90);

    m_pViewWidget = new ezQtOrbitCamViewWidget(this, &m_ViewConfig);
    m_pViewWidget->ConfigureOrbitCameraVolume(ezVec3(0), ezVec3(1.0f), ezVec3(-1, 0, 0));
    AddViewWidget(m_pViewWidget);
    ezQtViewWidgetContainer* pContainer = new ezQtViewWidgetContainer(this, m_pViewWidget, nullptr);

    setCentralWidget(pContainer);
  }

  {
    ezQtDocumentPanel* pPropertyPanel = new ezQtDocumentPanel(this);
    pPropertyPanel->setObjectName("TextureCubeAssetDockWidget");
    pPropertyPanel->setWindowTitle("Texture Properties");
    pPropertyPanel->show();

    ezQtPropertyGridWidget* pPropertyGrid = new ezQtPropertyGridWidget(pPropertyPanel, pDocument);
    pPropertyPanel->setWidget(pPropertyGrid);

    addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, pPropertyPanel);

    pDocument->GetSelectionManager()->SetSelection(pDocument->GetObjectManager()->GetRootObject()->GetChildren()[0]);
  }

  FinishWindowCreation();
}

void ezQtTextureCubeAssetDocumentWindow::InternalRedraw()
{
  ezEditorInputContext::UpdateActiveInputContext();
  SendRedrawMsg();
  ezQtEngineDocumentWindow::InternalRedraw();
}

void ezQtTextureCubeAssetDocumentWindow::SendRedrawMsg()
{
  // do not try to redraw while the process is crashed, it is obviously futile
  if (ezEditorEngineProcessConnection::GetSingleton()->IsProcessCrashed())
    return;

  {
    const ezTextureCubeAssetDocument* pDoc = static_cast<const ezTextureCubeAssetDocument*>(GetDocument());

    ezDocumentConfigMsgToEngine msg;
    msg.m_sWhatToDo = "ChannelMode";
    msg.m_iValue = pDoc->m_ChannelMode.GetValue();
    msg.m_fValue = pDoc->m_iTextureLod;

    GetEditorEngineConnection()->SendMessage(&msg);
  }

  for (auto pView : m_ViewWidgets)
  {
    pView->SetEnablePicking(false);
    pView->UpdateCameraInterpolation();
    pView->SyncToEngine();
  }
}
