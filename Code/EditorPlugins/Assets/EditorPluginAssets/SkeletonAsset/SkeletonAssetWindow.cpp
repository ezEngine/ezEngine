#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetStatusIndicator.moc.h>
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

    m_ViewConfig.m_Camera.LookAt(ezVec3(-1.6f, 0, 0), ezVec3(0, 0, 0), ezVec3(0, 0, 1));
    m_ViewConfig.ApplyPerspectiveSetting(90);

    m_pViewWidget = new ezQtOrbitCamViewWidget(this, &m_ViewConfig, true);
    m_pViewWidget->ConfigureRelative(ezVec3(0, 0, 1), ezVec3(5.0f), ezVec3(5, -2, 3), 2.0f);
    AddViewWidget(m_pViewWidget);
    pContainer = new ezQtViewWidgetContainer(this, m_pViewWidget, "SkeletonAssetViewToolBar");
    m_pDockManager->setCentralWidget(pContainer);
  }

  // Property Grid
  {
    ezQtDocumentPanel* pPropertyPanel = new ezQtDocumentPanel(this, pDocument);
    pPropertyPanel->setObjectName("SkeletonAssetDockWidget");
    pPropertyPanel->setWindowTitle("Skeleton Properties");
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

  // Tree View
  {
    ezQtDocumentPanel* pPanelTree = new ezQtSkeletonPanel(this, static_cast<ezSkeletonAssetDocument*>(pDocument));
    pPanelTree->show();

    m_pDockManager->addDockWidgetTab(ads::LeftDockWidgetArea, pPanelTree);
  }

  pDocument->Events().AddEventHandler(ezMakeDelegate(&ezQtSkeletonAssetDocumentWindow::SkeletonAssetEventHandler, this));

  GetDocument()->GetSelectionManager()->m_Events.AddEventHandler(ezMakeDelegate(&ezQtSkeletonAssetDocumentWindow::SelectionEventHandler, this));
  GetDocument()->GetObjectManager()->m_PropertyEvents.AddEventHandler(ezMakeDelegate(&ezQtSkeletonAssetDocumentWindow::PropertyEventHandler, this));
  GetDocument()->GetCommandHistory()->m_Events.AddEventHandler(ezMakeDelegate(&ezQtSkeletonAssetDocumentWindow::CommandEventHandler, this));

  FinishWindowCreation();
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
    msg.m_PayloadValue = pDoc->GetRenderBones();
    pDoc->SendMessageToEngine(&msg);
  }

  {
    ezSimpleDocumentConfigMsgToEngine msg;
    msg.m_sWhatToDo = "RenderColliders";
    msg.m_PayloadValue = pDoc->GetRenderColliders();
    pDoc->SendMessageToEngine(&msg);
  }

  {
    ezSimpleDocumentConfigMsgToEngine msg;
    msg.m_sWhatToDo = "RenderJoints";
    msg.m_PayloadValue = pDoc->GetRenderJoints();
    pDoc->SendMessageToEngine(&msg);
  }

  {
    ezSimpleDocumentConfigMsgToEngine msg;
    msg.m_sWhatToDo = "RenderSwingLimits";
    msg.m_PayloadValue = pDoc->GetRenderSwingLimits();
    pDoc->SendMessageToEngine(&msg);
  }

  {
    ezSimpleDocumentConfigMsgToEngine msg;
    msg.m_sWhatToDo = "RenderTwistLimits";
    msg.m_PayloadValue = pDoc->GetRenderTwistLimits();
    pDoc->SendMessageToEngine(&msg);
  }

  {
    ezSimpleDocumentConfigMsgToEngine msg;
    msg.m_sWhatToDo = "PreviewMesh";

    if (pDoc->GetRenderPreviewMesh())
      msg.m_sPayload = pDoc->GetProperties()->m_sPreviewMesh;
    else
      msg.m_sPayload = "";

    GetDocument()->SendMessageToEngine(&msg);
  }

  for (auto pView : m_ViewWidgets)
  {
    pView->SetEnablePicking(true);
    pView->UpdateCameraInterpolation();
    pView->SyncToEngine();
  }

  QueryObjectBBox();
}

void ezQtSkeletonAssetDocumentWindow::QueryObjectBBox(ezInt32 iPurpose /*= 0*/)
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

    case ezSelectionManagerEvent::Type::ChangedRuntimeOverrideSelection:
      // ignore
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
  if (e.m_sProperty == "LocalRotation" ||                                                 // joint offset rotation
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

  ezContiguousMemoryStreamStorage streamStorage;
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
  pDoc->WriteResource(memoryWriter, *pDoc->GetProperties()).AssertSuccess();
  msg.m_Data = ezArrayPtr<const ezUInt8>(streamStorage.GetData(), streamStorage.GetStorageSize32());

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
      m_pViewWidget->SetOrbitVolume(pMessage->m_vCenter, pMessage->m_vHalfExtents.CompMax(ezVec3(0.1f)));
    }
    else
    {
      // try again
      QueryObjectBBox(pMessage->m_iPurpose);
    }

    return;
  }

  ezQtEngineDocumentWindow::ProcessMessageEventHandler(pMsg);
}
