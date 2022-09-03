#include <EditorPluginKraut/EditorPluginKrautPCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/DocumentWindow/OrbitCamViewWidget.moc.h>
#include <EditorFramework/InputContexts/OrbitCameraContext.h>
#include <EditorPluginKraut/KrautTreeAsset/KrautTreeAssetWindow.moc.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <GuiFoundation/DockPanels/DocumentPanel.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <QBoxLayout>
#include <QComboBox>

ezQtKrautTreeAssetDocumentWindow::ezQtKrautTreeAssetDocumentWindow(ezAssetDocument* pDocument)
  : ezQtEngineDocumentWindow(pDocument)
{
  // Menu Bar
  {
    ezQtMenuBarActionMapView* pMenuBar = static_cast<ezQtMenuBarActionMapView*>(menuBar());
    ezActionContext context;
    context.m_sMapping = "KrautTreeAssetMenuBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pMenuBar->SetActionContext(context);
  }

  // Tool Bar
  {
    ezQtToolBarActionMapView* pToolBar = new ezQtToolBarActionMapView("Toolbar", this);
    ezActionContext context;
    context.m_sMapping = "KrautTreeAssetToolBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pToolBar->SetActionContext(context);
    pToolBar->setObjectName("KrautTreeAssetWindowToolBar");
    addToolBar(pToolBar);
  }

  // 3D View
  ezQtViewWidgetContainer* pContainer = nullptr;
  {
    SetTargetFramerate(25);

    m_ViewConfig.m_Camera.LookAt(ezVec3(-1.6, 0, 0), ezVec3(0, 0, 0), ezVec3(0, 0, 1));
    m_ViewConfig.ApplyPerspectiveSetting(90);

    m_pViewWidget = new ezQtOrbitCamViewWidget(this, &m_ViewConfig);
    m_pViewWidget->ConfigureOrbitCameraVolume(ezVec3(0, 0, 1), ezVec3(10.0f), ezVec3(-5, 1, 2));
    AddViewWidget(m_pViewWidget);
    pContainer = new ezQtViewWidgetContainer(this, m_pViewWidget, "MeshAssetViewToolBar");
    setCentralWidget(pContainer);
  }


  // Property Grid
  {
    ezDocumentObject* pRootObject = pDocument->GetObjectManager()->GetRootObject()->GetChildren()[0];

    ezDeque<const ezDocumentObject*> sel;
    sel.PushBack(pRootObject);

    ezQtDocumentPanel* pPropertyPanel = new ezQtDocumentPanel(this, pDocument);
    pPropertyPanel->setObjectName("KrautTreeAssetDockWidget");
    pPropertyPanel->setWindowTitle("Kraut Tree Properties");
    pPropertyPanel->show();

    QComboBox* pBranchTypeCombo = new QComboBox(pPropertyPanel);

    QTabWidget* pTabWidget = new QTabWidget(pPropertyPanel);

    QWidget* pGroup = new QWidget(pPropertyPanel);
    pGroup->setLayout(new QVBoxLayout);
    pGroup->layout()->addWidget(pBranchTypeCombo);
    pGroup->layout()->addWidget(pTabWidget);

    pPropertyPanel->setWidget(pGroup);

    ezQtPropertyGridWidget* pAssetProps = new ezQtPropertyGridWidget(pTabWidget, pDocument, false);
    pAssetProps->SetSelectionIncludeExcludeProperties(nullptr, "Materials;BT_Trunk1;BT_Trunk2;BT_Trunk3;BT_MainBranch1;BT_MainBranch2;BT_MainBranch3;BT_SubBranch1;BT_SubBranch2;BT_SubBranch3;BT_Twig1;BT_Twig2;BT_Twig3");
    pAssetProps->SetSelection(sel);
    pTabWidget->addTab(pAssetProps, "Asset");

    //ezQtPropertyGridWidget* pMaterialProps = new ezQtPropertyGridWidget(pTabWidget, pDocument, false);
    //pMaterialProps->SetSelectionIncludeExcludeProperties("Materials");
    //pMaterialProps->SetSelection(sel);
    //pTabWidget->addTab(pMaterialProps, "Materials");

    m_pBranchProps = new ezQtPropertyGridWidget(pTabWidget, pDocument, false);
    //m_pBranchProps->SetSelectionIncludeExcludeProperties("BT_Trunk1;BT_MainBranch1;BT_MainBranch2");
    //m_pBranchProps->SetSelection(sel);
    pTabWidget->addTab(m_pBranchProps, "Branch Type");

    addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, pPropertyPanel);

    pDocument->GetSelectionManager()->SetSelection(pDocument->GetObjectManager()->GetRootObject()->GetChildren()[0]);

    {
      connect(pBranchTypeCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(onBranchTypeSelected(int)));
      pBranchTypeCombo->addItem("Trunk");
      //pBranchTypeCombo->addItem("Trunk 2");
      //pBranchTypeCombo->addItem("Trunk 3");
      pBranchTypeCombo->addItem("Main Branch 1");
      pBranchTypeCombo->addItem("Main Branch 2");
      pBranchTypeCombo->addItem("Main Branch 3");
      pBranchTypeCombo->addItem("Sub Branch 1");
      pBranchTypeCombo->addItem("Sub Branch 2");
      pBranchTypeCombo->addItem("Sub Branch 3");
      pBranchTypeCombo->addItem("Twig 1");
      pBranchTypeCombo->addItem("Twig 2");
      pBranchTypeCombo->addItem("Twig 3");

      pBranchTypeCombo->setCurrentIndex(0);
    }
  }

  m_pAssetDoc = static_cast<ezKrautTreeAssetDocument*>(pDocument);

  FinishWindowCreation();

  QueryObjectBBox(0);

  GetDocument()->GetObjectManager()->m_PropertyEvents.AddEventHandler(ezMakeDelegate(&ezQtKrautTreeAssetDocumentWindow::PropertyEventHandler, this));
}

ezQtKrautTreeAssetDocumentWindow::~ezQtKrautTreeAssetDocumentWindow()
{
  GetDocument()->GetObjectManager()->m_PropertyEvents.RemoveEventHandler(ezMakeDelegate(&ezQtKrautTreeAssetDocumentWindow::PropertyEventHandler, this));

  RestoreResource();
}

void ezQtKrautTreeAssetDocumentWindow::SendRedrawMsg()
{
  // do not try to redraw while the process is crashed, it is obviously futile
  if (ezEditorEngineProcessConnection::GetSingleton()->IsProcessCrashed())
    return;

  for (auto pView : m_ViewWidgets)
  {
    pView->SetEnablePicking(false);
    pView->UpdateCameraInterpolation();
    pView->SyncToEngine();
  }

  QueryObjectBBox(-1);
}

void ezQtKrautTreeAssetDocumentWindow::QueryObjectBBox(ezInt32 iPurpose)
{
  ezQuerySelectionBBoxMsgToEngine msg;
  msg.m_uiViewID = 0xFFFFFFFF;
  msg.m_iPurpose = iPurpose;
  GetDocument()->SendMessageToEngine(&msg);
}

void ezQtKrautTreeAssetDocumentWindow::RestoreResource()
{
  ezRestoreResourceMsgToEngine msg;
  msg.m_sResourceType = "Kraut Tree";

  ezStringBuilder tmp;
  msg.m_sResourceID = ezConversionUtils::ToString(GetDocument()->GetGuid(), tmp);

  ezEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
}

void ezQtKrautTreeAssetDocumentWindow::UpdatePreview()
{
  if (ezEditorEngineProcessConnection::GetSingleton()->IsProcessCrashed())
    return;

  ezResourceUpdateMsgToEngine msg;
  msg.m_sResourceType = "Kraut Tree";

  ezStringBuilder tmp;
  msg.m_sResourceID = ezConversionUtils::ToString(GetDocument()->GetGuid(), tmp);

  ezContiguousMemoryStreamStorage streamStorage;
  ezMemoryStreamWriter memoryWriter(&streamStorage);

  // Write Path
  ezStringBuilder sAbsFilePath = GetDocument()->GetDocumentPath();
  sAbsFilePath.ChangeFileExtension("ezKrautTree");

  // Write Header
  memoryWriter << sAbsFilePath;
  const ezUInt64 uiHash = ezAssetCurator::GetSingleton()->GetAssetDependencyHash(GetDocument()->GetGuid());
  ezAssetFileHeader AssetHeader;
  AssetHeader.SetFileHashAndVersion(uiHash, GetDocument()->GetAssetTypeVersion());
  AssetHeader.Write(memoryWriter).AssertSuccess();

  // Write Asset Data
  GetKrautDocument()->WriteKrautAsset(memoryWriter);
  msg.m_Data = ezArrayPtr<const ezUInt8>(streamStorage.GetData(), streamStorage.GetStorageSize32());

  ezEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
}

void ezQtKrautTreeAssetDocumentWindow::InternalRedraw()
{
  ezEditorInputContext::UpdateActiveInputContext();
  SendRedrawMsg();
  ezQtEngineDocumentWindow::InternalRedraw();
}

void ezQtKrautTreeAssetDocumentWindow::ProcessMessageEventHandler(const ezEditorEngineDocumentMsg* pMsg)
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

void ezQtKrautTreeAssetDocumentWindow::PropertyEventHandler(const ezDocumentObjectPropertyEvent& e)
{
  if (e.m_sProperty == "DisplayRandomSeed")
  {
    ezSimpleDocumentConfigMsgToEngine msg;
    msg.m_sWhatToDo = "UpdateTree";
    msg.m_sPayload = "DisplayRandomSeed";
    msg.m_fPayload = static_cast<ezKrautTreeAssetDocument*>(GetDocument())->GetProperties()->m_uiRandomSeedForDisplay;

    GetDocument()->SendMessageToEngine(&msg);
  }
  else
  {
    UpdatePreview();
  }
}

void ezQtKrautTreeAssetDocumentWindow::onBranchTypeSelected(int index)
{
  if (m_pBranchProps == nullptr)
    return;

  ezDocumentObject* pRootObject = GetDocument()->GetObjectManager()->GetRootObject()->GetChildren()[0];

  ezDocumentObject* pSelected = nullptr;

  for (ezDocumentObject* pChild : pRootObject->GetChildren())
  {
    if (index == 0 && ezStringUtils::IsEqual(pChild->GetParentProperty(), "BT_Trunk1"))
    {
      pSelected = pChild;
      break;
    }
    //if (index == 1 && ezStringUtils::IsEqual(pChild->GetParentProperty(), "BT_Trunk2"))
    //{
    //  pSelected = pChild;
    //  break;
    //}
    //if (index == 2 && ezStringUtils::IsEqual(pChild->GetParentProperty(), "BT_Trunk3"))
    //{
    //  pSelected = pChild;
    //  break;
    //}
    if (index == 1 && ezStringUtils::IsEqual(pChild->GetParentProperty(), "BT_MainBranch1"))
    {
      pSelected = pChild;
      break;
    }
    if (index == 2 && ezStringUtils::IsEqual(pChild->GetParentProperty(), "BT_MainBranch2"))
    {
      pSelected = pChild;
      break;
    }
    if (index == 3 && ezStringUtils::IsEqual(pChild->GetParentProperty(), "BT_MainBranch3"))
    {
      pSelected = pChild;
      break;
    }
    if (index == 4 && ezStringUtils::IsEqual(pChild->GetParentProperty(), "BT_SubBranch1"))
    {
      pSelected = pChild;
      break;
    }
    if (index == 5 && ezStringUtils::IsEqual(pChild->GetParentProperty(), "BT_SubBranch2"))
    {
      pSelected = pChild;
      break;
    }
    if (index == 6 && ezStringUtils::IsEqual(pChild->GetParentProperty(), "BT_SubBranch3"))
    {
      pSelected = pChild;
      break;
    }
    if (index == 7 && ezStringUtils::IsEqual(pChild->GetParentProperty(), "BT_Twig1"))
    {
      pSelected = pChild;
      break;
    }
    if (index == 8 && ezStringUtils::IsEqual(pChild->GetParentProperty(), "BT_Twig2"))
    {
      pSelected = pChild;
      break;
    }
    if (index == 9 && ezStringUtils::IsEqual(pChild->GetParentProperty(), "BT_Twig3"))
    {
      pSelected = pChild;
      break;
    }
  }

  ezDeque<const ezDocumentObject*> sel;
  sel.PushBack(pSelected);
  GetDocument()->GetSelectionManager()->SetSelection(pSelected);
  m_pBranchProps->SetSelection(sel);
}
