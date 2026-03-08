#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetProcessor.h>
#include <EditorFramework/Assets/AssetStatusIndicator.moc.h>
#include <EditorFramework/DocumentWindow/OrbitCamViewWidget.moc.h>
#include <EditorFramework/InputContexts/OrbitCameraContext.h>
#include <EditorPluginAssets/MeshAsset/MeshAssetWindow.moc.h>
#include <EditorPluginAssets/MeshAsset/MeshEditorContext.h>
#include <Foundation/Utilities/ConversionUtils.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <GuiFoundation/DockPanels/DocumentPanel.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <SharedPluginAssets/Common/Messages.h>

ezQtMeshAssetDocumentWindow::ezQtMeshAssetDocumentWindow(ezMeshAssetDocument* pDocument)
  : ezQtEngineDocumentWindow(pDocument)
{
  GetDocument()->GetObjectManager()->m_PropertyEvents.AddEventHandler(ezMakeDelegate(&ezQtMeshAssetDocumentWindow::PropertyEventHandler, this));

  // Menu Bar
  {
    ezQtMenuBarActionMapView* pMenuBar = static_cast<ezQtMenuBarActionMapView*>(menuBar());
    ezActionContext context;
    context.m_sMapping = "MeshAssetMenuBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pMenuBar->SetActionContext(context);
  }

  // Tool Bar
  {
    ezQtToolBarActionMapView* pToolBar = new ezQtToolBarActionMapView("Toolbar", this);
    ezActionContext context;
    context.m_sMapping = "MeshAssetToolBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pToolBar->SetActionContext(context);
    pToolBar->setObjectName("MeshAssetWindowToolBar");
    addToolBar(pToolBar);
  }

  // 3D View
  ezQtViewWidgetContainer* pContainer = nullptr;
  {
    SetTargetFramerate(25);

    m_ViewConfig.m_Camera.LookAt(ezVec3(-1.6f, 0, 0), ezVec3(0, 0, 0), ezVec3(0, 0, 1));
    m_ViewConfig.ApplyPerspectiveSetting(90);

    m_pViewWidget = new ezQtOrbitCamViewWidget(this, &m_ViewConfig);
    m_pViewWidget->ConfigureRelative(ezVec3(0, 0, 1), ezVec3(5.0f), ezVec3(5, -2, 3), 2.0f);
    AddViewWidget(m_pViewWidget);

    m_pMeshEditorInputContext = EZ_DEFAULT_NEW(ezMeshEditorInputContext, this, m_pViewWidget);

    m_pCameraFlyContext = EZ_DEFAULT_NEW(ezCameraMoveContext, this, m_pViewWidget);
    m_pCameraFlyContext->SetCamera(&m_ViewConfig.m_Camera);
    m_pCameraFlyContext->LoadState();

    // Default mode is orbit: [OrbitCamera, MeshEditorInputContext]
    // The orbit camera was already pushed by ezQtOrbitCamViewWidget; just append our context.
    m_pViewWidget->m_InputContexts.PushBack(m_pMeshEditorInputContext.Borrow());

    pContainer = new ezQtViewWidgetContainer(GetContainerWindow()->GetDockManager(), this, m_pViewWidget, "MeshAssetViewToolBar");
    m_pDockManager->setCentralWidget(pContainer);
  }

  // Property Grid
  {
    ezQtDocumentPanel* pPropertyPanel = new ezQtDocumentPanel(GetContainerWindow()->GetDockManager(), this, pDocument);
    pPropertyPanel->setObjectName("MeshAssetDockWidget");
    pPropertyPanel->setWindowTitle("Mesh Properties");

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

  UpdatePreview();

  m_pHighlightTimer = new QTimer();
  connect(m_pHighlightTimer, &QTimer::timeout, this, &ezQtMeshAssetDocumentWindow::HighlightTimer);
  m_pHighlightTimer->setInterval(500);
  m_pHighlightTimer->start();
}

ezQtMeshAssetDocumentWindow::~ezQtMeshAssetDocumentWindow()
{
  m_pHighlightTimer->stop();

  GetDocument()->GetObjectManager()->m_PropertyEvents.RemoveEventHandler(ezMakeDelegate(&ezQtMeshAssetDocumentWindow::PropertyEventHandler, this));
}

ezMeshAssetDocument* ezQtMeshAssetDocumentWindow::GetMeshDocument()
{
  return static_cast<ezMeshAssetDocument*>(GetDocument());
}

void ezQtMeshAssetDocumentWindow::SetCameraMode(int iMode)
{
  if (m_iCameraMode == iMode)
    return;
  m_iCameraMode = iMode;

  // Rebuild input context list: camera context first, Ctrl+MMB handler second.
  m_pViewWidget->m_InputContexts.Clear();
  if (iMode == 0) // Orbit
    m_pViewWidget->m_InputContexts.PushBack(m_pViewWidget->GetOrbitCamera());
  else            // Free fly
    m_pViewWidget->m_InputContexts.PushBack(m_pCameraFlyContext.Borrow());
  m_pViewWidget->m_InputContexts.PushBack(m_pMeshEditorInputContext.Borrow());
}

void ezQtMeshAssetDocumentWindow::SendRedrawMsg()
{
  // do not try to redraw while the process is crashed, it is obviously futile
  if (ezEditorEngineProcessConnection::GetSingleton()->IsProcessCrashed())
    return;

  for (auto pView : m_ViewWidgets)
  {
    // Picking must always be active to support the hover material display and Ctrl+MMB material opening.
    pView->SetEnablePicking(true);
    pView->UpdateCameraInterpolation();
    pView->SyncToEngine();
  }

  QueryObjectBBox();
}

void ezQtMeshAssetDocumentWindow::QueryObjectBBox(ezInt32 iPurpose /*= 0*/)
{
  ezQuerySelectionBBoxMsgToEngine msg;
  msg.m_uiViewID = 0xFFFFFFFF;
  msg.m_iPurpose = iPurpose;
  GetDocument()->SendMessageToEngine(&msg);
}

void ezQtMeshAssetDocumentWindow::PropertyEventHandler(const ezDocumentObjectPropertyEvent& e)
{
  // if (e.m_sProperty == "Resource") // any material change
  {
    UpdatePreview();
  }
}

bool ezQtMeshAssetDocumentWindow::UpdatePreview()
{
  if (ezEditorEngineProcessConnection::GetSingleton()->IsProcessCrashed())
    return false;

  if (GetMeshDocument()->GetProperties() == nullptr)
    return false;

  const auto& materials = GetMeshDocument()->GetProperties()->m_Slots;

  ezEditorEngineSetMaterialsMsg msg;
  msg.m_Materials.SetCount(materials.GetCount());
  msg.m_SlotNames.SetCount(materials.GetCount());

  ezUInt32 uiSlot = 0;
  bool bHighlighted = false;

  for (ezUInt32 i = 0; i < materials.GetCount(); ++i)
  {
    msg.m_Materials[i] = materials[i].m_sResource;

    if (materials[i].m_bHighlight)
    {
      if (uiSlot == m_uiHighlightSlots)
      {
        bHighlighted = true;
        msg.m_Materials[i] = "Editor/Materials/HighlightMesh.ezMaterial";
      }

      ++uiSlot;
    }

    // Compute a readable display name: resolve GUID to asset filename, fall back to slot label
    ezStringBuilder sDisplayName = materials[i].m_sLabel;
    if (!materials[i].m_sResource.IsEmpty())
    {
      if (ezConversionUtils::IsStringUuid(materials[i].m_sResource))
      {
        const ezUuid guid = ezConversionUtils::ConvertStringToUuid(materials[i].m_sResource);
        auto pSubAsset = ezAssetCurator::GetSingleton()->GetSubAsset(guid);
        if (pSubAsset)
          sDisplayName = pSubAsset->m_pAssetInfo->m_Path.GetAbsolutePath().GetFileName();
      }
      else
      {
        ezStringView sv(materials[i].m_sResource);
        sDisplayName = sv.GetFileName();
      }
    }
    msg.m_SlotNames[i] = sDisplayName;
  }

  GetEditorEngineConnection()->SendMessage(&msg);

  return bHighlighted;
}

void ezQtMeshAssetDocumentWindow::InternalRedraw()
{
  ezEditorInputContext::UpdateActiveInputContext();
  SendRedrawMsg();
  ezQtEngineDocumentWindow::InternalRedraw();
}

void ezQtMeshAssetDocumentWindow::ProcessMessageEventHandler(const ezEditorEngineDocumentMsg* pMsg)
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

void ezQtMeshAssetDocumentWindow::HighlightTimer()
{
  if (m_uiHighlightSlots & EZ_BIT(31))
    m_uiHighlightSlots &= ~EZ_BIT(31);
  else
    m_uiHighlightSlots |= EZ_BIT(31);

  if (m_uiHighlightSlots & EZ_BIT(31))
  {
    UpdatePreview();
  }
  else
  {
    if (UpdatePreview())
    {
      ++m_uiHighlightSlots;
    }
    else
    {
      m_uiHighlightSlots = EZ_BIT(31);
    }
  }
}
