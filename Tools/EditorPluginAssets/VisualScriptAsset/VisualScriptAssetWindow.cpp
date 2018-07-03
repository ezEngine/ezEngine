#include <PCH.h>
#include <EditorPluginAssets/VisualScriptAsset/VisualScriptAssetWindow.moc.h>
#include <EditorPluginAssets/VisualScriptAsset/VisualScriptGraphQt.moc.h>
#include <EditorPluginAssets/VisualScriptAsset/VisualScriptAsset.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <GuiFoundation/DockPanels/DocumentPanel.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <GuiFoundation/NodeEditor/NodeScene.moc.h>
#include <GuiFoundation/NodeEditor/NodeView.moc.h>
#include <GuiFoundation/Dialogs/PickDocumentObjectDlg.moc.h>
#include <GameEngine/VisualScript/VisualScriptComponent.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>
#include <EditorPluginAssets/VisualScriptAsset/VisualScriptAsset.h>

#include <QLabel>
#include <QLayout>
#include <EditorFramework/Preferences/Preferences.h>

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualScriptPreferences, 1, ezRTTIDefaultAllocator<ezVisualScriptPreferences>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("DebugObject", m_DebugObject)->AddAttributes(new ezHiddenAttribute()),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezVisualScriptPreferences::ezVisualScriptPreferences() 
  : ezPreferences(ezPreferences::Domain::Document, "Visual Script")
{ 
}

//////////////////////////////////////////////////////////////////////////

ezQtVisualScriptAssetDocumentWindow::ezQtVisualScriptAssetDocumentWindow(ezDocument* pDocument, const ezDocumentObject* pOpenContext)
  : ezQtDocumentWindow(pDocument)
{

  // Menu Bar
  {
    ezQtMenuBarActionMapView* pMenuBar = static_cast<ezQtMenuBarActionMapView*>(menuBar());
    ezActionContext context;
    context.m_sMapping = "VisualScriptAssetMenuBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pMenuBar->SetActionContext(context);
  }

  // Tool Bar
  {
    ezQtToolBarActionMapView* pToolBar = new ezQtToolBarActionMapView("Toolbar", this);
    ezActionContext context;
    context.m_sMapping = "VisualScriptAssetToolBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pToolBar->SetActionContext(context);
    pToolBar->setObjectName("VisualScriptAssetWindowToolBar");
    addToolBar(pToolBar);
  }

  m_pScene = new ezQtVisualScriptAssetScene(this);
  m_pScene->SetDocumentNodeManager(static_cast<const ezDocumentNodeManager*>(pDocument->GetObjectManager()));
  m_pView = new ezQtNodeView(this);
  m_pView->SetScene(m_pScene);
  setCentralWidget(m_pView);

  {
    ezQtDocumentPanel* pPropertyPanel = new ezQtDocumentPanel(this);
    pPropertyPanel->setObjectName("VisualScriptAssetDockWidget");
    pPropertyPanel->setWindowTitle("Node Properties");
    pPropertyPanel->show();

    ezQtPropertyGridWidget* pPropertyGrid = new ezQtPropertyGridWidget(pPropertyPanel, pDocument);
    pPropertyPanel->setWidget(pPropertyGrid);

    addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, pPropertyPanel);
  }

  static_cast<ezVisualScriptAssetDocument*>(pDocument)->m_ActivityEvents.AddEventHandler(ezMakeDelegate(&ezQtVisualScriptAssetScene::VisualScriptActivityEventHandler, m_pScene));
  static_cast<ezVisualScriptAssetDocument*>(pDocument)->m_InterDocumentMessages.AddEventHandler(ezMakeDelegate(&ezQtVisualScriptAssetScene::VisualScriptInterDocumentMessageHandler, m_pScene));

  GetDocument()->GetSelectionManager()->m_Events.AddEventHandler(ezMakeDelegate(&ezQtVisualScriptAssetDocumentWindow::SelectionEventHandler, this));

  if (pOpenContext != nullptr)
  {
    m_pScene->SetDebugObject(pOpenContext->GetGuid());
  }
  else
  {
    ezVisualScriptPreferences* pPreferences = ezPreferences::QueryPreferences<ezVisualScriptPreferences>(GetDocument());

    m_pScene->SetDebugObject(pPreferences->m_DebugObject);
  }

  FinishWindowCreation();

  SelectionEventHandler(ezSelectionManagerEvent());
}

ezQtVisualScriptAssetDocumentWindow::~ezQtVisualScriptAssetDocumentWindow()
{
  if (GetDocument() != nullptr)
  {
    GetDocument()->GetSelectionManager()->m_Events.RemoveEventHandler(ezMakeDelegate(&ezQtVisualScriptAssetDocumentWindow::SelectionEventHandler, this));

    GetVisualScriptDocument()->m_ActivityEvents.RemoveEventHandler(ezMakeDelegate(&ezQtVisualScriptAssetScene::VisualScriptActivityEventHandler, m_pScene));
    GetVisualScriptDocument()->m_InterDocumentMessages.RemoveEventHandler(ezMakeDelegate(&ezQtVisualScriptAssetScene::VisualScriptInterDocumentMessageHandler, m_pScene));
  }
}

ezVisualScriptAssetDocument* ezQtVisualScriptAssetDocumentWindow::GetVisualScriptDocument()
{
  return static_cast<ezVisualScriptAssetDocument*>(GetDocument());
}

void ezQtVisualScriptAssetDocumentWindow::PickDebugTarget()
{
  ezGatherObjectsOfTypeMsgInterDoc msg;
  msg.m_pType = ezGetStaticRTTI<ezVisualScriptComponent>();

  GetDocument()->BroadcastInterDocumentMessage(&msg, GetDocument());

  ezHybridArray<ezQtPickDocumentObjectDlg::Element, 16> objects;

  const ezUuid scriptGuid = GetDocument()->GetGuid();
  ezStringBuilder sScriptGuid;
  ezConversionUtils::ToString(scriptGuid, sScriptGuid);

  for (auto& res : msg.m_Results)
  {
    const ezDocumentObject* pObject = res.m_pDocument->GetObjectManager()->GetObject(res.m_ObjectGuid);

    if (pObject == nullptr)
      continue;

    const ezVariant varScript = pObject->GetTypeAccessor().GetValue("Script");
    if (!varScript.IsValid() || !varScript.IsA<ezString>())
      continue;

    if (varScript.Get<ezString>() != sScriptGuid)
      continue;

    auto& obj = objects.ExpandAndGetRef();
    obj.m_pObject = pObject;
    obj.m_sDisplayName = res.m_sDisplayName;
  }


  ezQtPickDocumentObjectDlg dlg(this, objects, m_pScene->GetDebugObject());
  dlg.exec();

  if (dlg.m_pPickedObject != nullptr)
  {
    ezVisualScriptPreferences* pPreferences = ezPreferences::QueryPreferences<ezVisualScriptPreferences>(GetDocument());
    pPreferences->m_DebugObject = dlg.m_pPickedObject->GetGuid();

    m_pScene->SetDebugObject(pPreferences->m_DebugObject);
  }
}

void ezQtVisualScriptAssetDocumentWindow::SelectionEventHandler(const ezSelectionManagerEvent& e)
{
  if (GetDocument()->GetSelectionManager()->IsSelectionEmpty())
  {
    // delayed execution
    QTimer::singleShot(1, [this]()
    {
      GetDocument()->GetSelectionManager()->SetSelection(GetVisualScriptDocument()->GetPropertyObject());
    });
  }
}

