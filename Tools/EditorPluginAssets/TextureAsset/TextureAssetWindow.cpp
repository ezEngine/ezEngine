#include <PCH.h>
#include <EditorPluginAssets/TextureAsset/TextureAssetWindow.moc.h>
#include <EditorPluginAssets/TextureAsset/TextureAssetObjects.h>
#include <EditorPluginAssets/TextureAsset/TextureAsset.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <GuiFoundation/Widgets/ImageWidget.moc.h>
#include <GuiFoundation/DockPanels/DocumentPanel.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <CoreUtils/Image/ImageConversion.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorPluginAssets/TextureAsset/TextureViewWidget.moc.h>
#include <EditorFramework/InputContexts/EditorInputContext.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/Action/ActionManager.h>

////////////////////////////////////////////////////////////////////////
// ezTextureChannelModeAction
////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTextureChannelModeAction, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE

ezTextureChannelModeAction::ezTextureChannelModeAction(const ezActionContext& context, const char* szName, const char* szIconPath)
  : ezEnumerationMenuAction(context, szName, szIconPath)
{
  InitEnumerationType(ezGetStaticRTTI<ezTextureChannelMode>());
}

ezInt64 ezTextureChannelModeAction::GetValue() const
{
  return static_cast<const ezTextureAssetDocument*>(m_Context.m_pDocument)->m_ChannelMode.GetValue();
}

void ezTextureChannelModeAction::Execute(const ezVariant& value)
{
  ((ezTextureAssetDocument*)m_Context.m_pDocument)->m_ChannelMode.SetValue(value.ConvertTo<ezInt32>());
}

//////////////////////////////////////////////////////////////////////////

ezActionDescriptorHandle ezTextureAssetActions::s_hTextureChannelMode;

void ezTextureAssetActions::RegisterActions()
{
  s_hTextureChannelMode = EZ_REGISTER_DYNAMIC_MENU("TextureAsset.ChannelMode", ezTextureChannelModeAction, ":/EditorFramework/Icons/RenderMode.png");
}

void ezTextureAssetActions::UnregisterActions()
{
  ezActionManager::UnregisterAction(s_hTextureChannelMode);
}

void ezTextureAssetActions::MapActions(const char* szMapping, const char* szPath)
{
  ezActionMap* pMap = ezActionMapManager::GetActionMap(szMapping);
  EZ_ASSERT_DEV(pMap != nullptr, "The given mapping ('%s') does not exist, mapping the actions failed!", szMapping);

  pMap->MapAction(s_hTextureChannelMode, szPath, 15.0f);
}


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
    pMenuBar->SetActionContext(context);
  }

  // Tool Bar
  {
    ezQtToolBarActionMapView* pToolBar = new ezQtToolBarActionMapView("Toolbar", this);
    ezActionContext context;
    context.m_sMapping = "TextureAssetToolBar";
    context.m_pDocument = pDocument;
    pToolBar->SetActionContext(context);
    pToolBar->setObjectName("TextureAssetWindowToolBar");
    addToolBar(pToolBar);
  }

  // 3D View
  {
    SetTargetFramerate(25);

    m_ViewConfig.m_Camera.LookAt(ezVec3(-2, 0, 0), ezVec3(0, 0, 0), ezVec3(0, 0, 1));
    m_ViewConfig.ApplyPerspectiveSetting(90);

    m_pViewWidget = new ezQtTextureViewWidget(nullptr, this, &m_ViewConfig);

    ezQtViewWidgetContainer* pContainer = new ezQtViewWidgetContainer(this, m_pViewWidget, nullptr);

    setCentralWidget(pContainer);
  }

  {
    ezQtDocumentPanel* pPropertyPanel = new ezQtDocumentPanel(this);
    pPropertyPanel->setObjectName("TextureAssetDockWidget");
    pPropertyPanel->setWindowTitle("Texture Properties");
    pPropertyPanel->show();

    ezQtPropertyGridWidget* pPropertyGrid = new ezQtPropertyGridWidget(pPropertyPanel, pDocument);
    pPropertyPanel->setWidget(pPropertyGrid);

    addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, pPropertyPanel);

    pDocument->GetSelectionManager()->SetSelection(pDocument->GetObjectManager()->GetRootObject()->GetChildren()[0]);

  }

  FinishWindowCreation();
}

void ezQtTextureAssetDocumentWindow::InternalRedraw()
{
  ezQtEngineDocumentWindow::InternalRedraw();

  ezEditorInputContext::UpdateActiveInputContext();

  SendRedrawMsg();
}

void ezQtTextureAssetDocumentWindow::SendRedrawMsg()
{
  // do not try to redraw while the process is crashed, it is obviously futile
  if (ezEditorEngineProcessConnection::GetSingleton()->IsProcessCrashed())
    return;

  {
    ezSceneSettingsMsgToEngine msg;
    msg.m_fGizmoScale = 0;
    GetEditorEngineConnection()->SendMessage(&msg);
  }

  {
    ezDocumentConfigMsgToEngine msg;
    msg.m_sWhatToDo = "ChannelMode";
    msg.m_iValue = static_cast<const ezTextureAssetDocument*>(GetDocument())->m_ChannelMode.GetValue();

    GetEditorEngineConnection()->SendMessage(&msg);
  }

  for (auto pView : m_ViewWidgets)
  {
    pView->SetEnablePicking(false);
    pView->UpdateCameraInterpolation();
    pView->SyncToEngine();
  }

  {
    ezSyncWithProcessMsgToEngine sm;
    ezEditorEngineProcessConnection::GetSingleton()->SendMessage(&sm);

    ezEditorEngineProcessConnection::GetSingleton()->WaitForMessage(ezGetStaticRTTI<ezSyncWithProcessMsgToEditor>(), ezTime::Seconds(2.0));
  }
}
