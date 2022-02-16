#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/DocumentWindow/OrbitCamViewWidget.moc.h>
#include <EditorFramework/InputContexts/OrbitCameraContext.h>
#include <EditorFramework/InputContexts/SelectionContext.h>
#include <EditorPluginAssets/SkeletonAsset/SkeletonAsset.h>
#include <EditorPluginAssets/SkeletonAsset/SkeletonAssetWindow.moc.h>
#include <EditorPluginAssets/SkeletonAsset/SkeletonPanel.moc.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>

ezQtSkeletonAssetDocumentWindow::ezQtSkeletonAssetDocumentWindow(ezSkeletonAssetDocument* pDocument)
  : ezQtEngineDocumentWindow(pDocument)
{
  // Menu Bar
  {
    ezQtMenuBarActionMapView* pMenuBar = static_cast<ezQtMenuBarActionMapView*>(menuBar());
    ezActionContext context;
    context.m_sMapping = "SkeletonAssetMenuBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pMenuBar->SetActionContext(context);
  }

  // Tool Bar
  {
    ezQtToolBarActionMapView* pToolBar = new ezQtToolBarActionMapView("Toolbar", this);
    ezActionContext context;
    context.m_sMapping = "SkeletonAssetToolBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pToolBar->SetActionContext(context);
    pToolBar->setObjectName("SkeletonAssetWindowToolBar");
    addToolBar(pToolBar);
  }

  // 3D View
  ezQtViewWidgetContainer* pContainer = nullptr;
  {
    SetTargetFramerate(25);

    m_ViewConfig.m_Camera.LookAt(ezVec3(-1.6, 0, 0), ezVec3(0, 0, 0), ezVec3(0, 0, 1));
    m_ViewConfig.ApplyPerspectiveSetting(90);

    m_pViewWidget = new ezQtOrbitCamViewWidget(this, &m_ViewConfig, true);
    m_pViewWidget->ConfigureOrbitCameraVolume(ezVec3(0, 0, 1), ezVec3(10.0f), ezVec3(-5, 1, 2));
    AddViewWidget(m_pViewWidget);
    pContainer = new ezQtViewWidgetContainer(this, m_pViewWidget, "SkeletonAssetViewToolBar");
    setCentralWidget(pContainer);
  }

  // Property Grid
  {
    ezQtDocumentPanel* pPropertyPanel = new ezQtDocumentPanel(this, pDocument);
    pPropertyPanel->setObjectName("SkeletonAssetDockWidget");
    pPropertyPanel->setWindowTitle("Skeleton Properties");
    pPropertyPanel->show();

    ezQtPropertyGridWidget* pPropertyGrid = new ezQtPropertyGridWidget(pPropertyPanel, pDocument);
    pPropertyPanel->setWidget(pPropertyGrid);

    addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, pPropertyPanel);

    pDocument->GetSelectionManager()->SetSelection(pDocument->GetObjectManager()->GetRootObject()->GetChildren()[0]);
  }

  // Tree View
  {
    ezQtDocumentPanel* pPanelTree = new ezQtSkeletonPanel(this, static_cast<ezSkeletonAssetDocument*>(pDocument));
    pPanelTree->show();

    addDockWidget(Qt::DockWidgetArea::LeftDockWidgetArea, pPanelTree);
  }

  pDocument->Events().AddEventHandler(ezMakeDelegate(&ezQtSkeletonAssetDocumentWindow::SkeletonAssetEventHandler, this));

  GetDocument()->GetSelectionManager()->m_Events.AddEventHandler(ezMakeDelegate(&ezQtSkeletonAssetDocumentWindow::SelectionEventHandler, this));
  GetDocument()->GetObjectManager()->m_PropertyEvents.AddEventHandler(ezMakeDelegate(&ezQtSkeletonAssetDocumentWindow::PropertyEventHandler, this));
  GetDocument()->GetCommandHistory()->m_Events.AddEventHandler(ezMakeDelegate(&ezQtSkeletonAssetDocumentWindow::CommandEventHandler, this));

  FinishWindowCreation();

  QueryObjectBBox(0);
}

ezQtSkeletonAssetDocumentWindow::~ezQtSkeletonAssetDocumentWindow()
{
  static_cast<ezSkeletonAssetDocument*>(GetDocument())->Events().RemoveEventHandler(ezMakeDelegate(&ezQtSkeletonAssetDocumentWindow::SkeletonAssetEventHandler, this));

  GetDocument()->GetCommandHistory()->m_Events.RemoveEventHandler(ezMakeDelegate(&ezQtSkeletonAssetDocumentWindow::CommandEventHandler, this));
  GetDocument()->GetSelectionManager()->m_Events.RemoveEventHandler(ezMakeDelegate(&ezQtSkeletonAssetDocumentWindow::SelectionEventHandler, this));
  GetDocument()->GetObjectManager()->m_PropertyEvents.RemoveEventHandler(ezMakeDelegate(&ezQtSkeletonAssetDocumentWindow::PropertyEventHandler, this));

  RestoreResource();
}

ezSkeletonAssetDocument* ezQtSkeletonAssetDocumentWindow::GetSkeletonDocument()
{
  return static_cast<ezSkeletonAssetDocument*>(GetDocument());
}

void ezQtSkeletonAssetDocumentWindow::SendRedrawMsg()
{
  // do not try to redraw while the process is crashed, it is obviously futile
  if (ezEditorEngineProcessConnection::GetSingleton()->IsProcessCrashed())
    return;

  auto* pDoc = GetSkeletonDocument();

  {
    ezSimpleDocumentConfigMsgToEngine msg;
    msg.m_sWhatToDo = "RenderBones";
    msg.m_fPayload = pDoc->GetRenderBones() ? 1.0f : 0.0f;
    pDoc->SendMessageToEngine(&msg);
  }

  {
    ezSimpleDocumentConfigMsgToEngine msg;
    msg.m_sWhatToDo = "RenderColliders";
    msg.m_fPayload = pDoc->GetRenderColliders() ? 1.0f : 0.0f;
    pDoc->SendMessageToEngine(&msg);
  }

  {
    ezSimpleDocumentConfigMsgToEngine msg;
    msg.m_sWhatToDo = "RenderJoints";
    msg.m_fPayload = pDoc->GetRenderJoints() ? 1.0f : 0.0f;
    pDoc->SendMessageToEngine(&msg);
  }

  {
    ezSimpleDocumentConfigMsgToEngine msg;
    msg.m_sWhatToDo = "RenderSwingLimits";
    msg.m_fPayload = pDoc->GetRenderSwingLimits() ? 1.0f : 0.0f;
    pDoc->SendMessageToEngine(&msg);
  }

  {
    ezSimpleDocumentConfigMsgToEngine msg;
    msg.m_sWhatToDo = "RenderTwistLimits";
    msg.m_fPayload = pDoc->GetRenderTwistLimits() ? 1.0f : 0.0f;
    pDoc->SendMessageToEngine(&msg);
  }

  for (auto pView : m_ViewWidgets)
  {
    pView->SetEnablePicking(true);
    pView->UpdateCameraInterpolation();
    pView->SyncToEngine();
  }

  QueryObjectBBox(-1);
}

void ezQtSkeletonAssetDocumentWindow::QueryObjectBBox(ezInt32 iPurpose)
{
  ezQuerySelectionBBoxMsgToEngine msg;
  msg.m_uiViewID = 0xFFFFFFFF;
  msg.m_iPurpose = iPurpose;
  GetDocument()->SendMessageToEngine(&msg);
}


void ezQtSkeletonAssetDocumentWindow::SelectionEventHandler(const ezSelectionManagerEvent& e)
{
  ezStringBuilder filter;

  switch (e.m_Type)
  {
    case ezSelectionManagerEvent::Type::SelectionCleared:
    case ezSelectionManagerEvent::Type::SelectionSet:
    case ezSelectionManagerEvent::Type::ObjectAdded:
    case ezSelectionManagerEvent::Type::ObjectRemoved:
    {
      const auto& sel = GetDocument()->GetSelectionManager()->GetSelection();

      for (auto pObj : sel)
      {
        ezVariant name = pObj->GetTypeAccessor().GetValue("Name");
        if (name.IsValid() && name.CanConvertTo<ezString>())
        {
          filter.Append(name.ConvertTo<ezString>().GetData(), ";");
        }
      }

      ezSimpleDocumentConfigMsgToEngine msg;
      msg.m_sWhatToDo = "HighlightBones";
      msg.m_sPayload = filter;

      GetDocument()->SendMessageToEngine(&msg);
    }
    break;
  }
}

void ezQtSkeletonAssetDocumentWindow::SkeletonAssetEventHandler(const ezSkeletonAssetEvent& e)
{
  if (e.m_Type == ezSkeletonAssetEvent::Transformed)
  {
    SendLiveResourcePreview();
  }
}

void ezQtSkeletonAssetDocumentWindow::PropertyEventHandler(const ezDocumentObjectPropertyEvent& e)
{
  // additionally do live updates for these specific properties
  if (
    e.m_sProperty == "Offset" || e.m_sProperty == "Rotation" ||                         // all shapes
    e.m_sProperty == "Radius" || e.m_sProperty == "Length" ||                           // sphere and capsule
    e.m_sProperty == "Width" || e.m_sProperty == "Thickness" ||                         // box
    e.m_sProperty == "SwingLimitY" || e.m_sProperty == "SwingLimitZ" ||                 // joint swing limit
    e.m_sProperty == "TwistLimitHalfAngle" || e.m_sProperty == "TwistLimitCenterAngle") // joint twist limit
  {
    SendLiveResourcePreview();
  }
}

void ezQtSkeletonAssetDocumentWindow::CommandEventHandler(const ezCommandHistoryEvent& e)
{
  if (e.m_Type == ezCommandHistoryEvent::Type::TransactionEnded || e.m_Type == ezCommandHistoryEvent::Type::UndoEnded || e.m_Type == ezCommandHistoryEvent::Type::RedoEnded)
  {
    SendLiveResourcePreview();
  }
}

void ezQtSkeletonAssetDocumentWindow::SendLiveResourcePreview()
{
  if (ezEditorEngineProcessConnection::GetSingleton()->IsProcessCrashed())
    return;

  ezSkeletonAssetDocument* pDoc = ezDynamicCast<ezSkeletonAssetDocument*>(GetDocument());

  if (pDoc->m_bIsTransforming)
    return;

  ezResourceUpdateMsgToEngine msg;
  msg.m_sResourceType = "Skeleton";

  ezStringBuilder tmp;
  msg.m_sResourceID = ezConversionUtils::ToString(GetDocument()->GetGuid(), tmp);

  ezMemoryStreamStorage streamStorage;
  ezMemoryStreamWriter memoryWriter(&streamStorage);


  // Write Path
  ezStringBuilder sAbsFilePath = pDoc->GetDocumentPath();
  sAbsFilePath.ChangeFileExtension("ezSkeleton");

  // Write Header
  memoryWriter << sAbsFilePath;
  const ezUInt64 uiHash = ezAssetCurator::GetSingleton()->GetAssetDependencyHash(pDoc->GetGuid());
  ezAssetFileHeader AssetHeader;
  AssetHeader.SetFileHashAndVersion(uiHash, pDoc->GetAssetTypeVersion());
  AssetHeader.Write(memoryWriter).IgnoreResult();

  // Write Asset Data
  pDoc->WriteResource(memoryWriter);
  msg.m_Data = ezArrayPtr<const ezUInt8>(streamStorage.GetData(), streamStorage.GetStorageSize());

  ezEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
}

void ezQtSkeletonAssetDocumentWindow::RestoreResource()
{
  ezRestoreResourceMsgToEngine msg;
  msg.m_sResourceType = "Skeleton";

  ezStringBuilder tmp;
  msg.m_sResourceID = ezConversionUtils::ToString(GetDocument()->GetGuid(), tmp);

  ezEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
}

void ezQtSkeletonAssetDocumentWindow::InternalRedraw()
{
  ezEditorInputContext::UpdateActiveInputContext();
  SendRedrawMsg();
  ezQtEngineDocumentWindow::InternalRedraw();
}

void ezQtSkeletonAssetDocumentWindow::ProcessMessageEventHandler(const ezEditorEngineDocumentMsg* pMsg)
{
  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<ezQuerySelectionBBoxResultMsgToEditor>())
  {
    const ezQuerySelectionBBoxResultMsgToEditor* pMessage = static_cast<const ezQuerySelectionBBoxResultMsgToEditor*>(pMsg);

    if (pMessage->m_vCenter.IsValid() && pMessage->m_vHalfExtents.IsValid())
    {
      const ezVec3 vHalfExtents = pMessage->m_vHalfExtents.CompMax(ezVec3(0.1f));

      m_pViewWidget->GetOrbitCamera()->SetOrbitVolume(pMessage->m_vCenter, vHalfExtents * 2.0f, pMessage->m_vCenter + ezVec3(5, -2, 3) * vHalfExtents.GetLength() * 0.3f, pMessage->m_iPurpose == 0);
    }
    else if (pMessage->m_iPurpose == 0)
    {
      // try again
      QueryObjectBBox(pMessage->m_iPurpose);
    }

    return;
  }

  ezQtEngineDocumentWindow::ProcessMessageEventHandler(pMsg);
}
