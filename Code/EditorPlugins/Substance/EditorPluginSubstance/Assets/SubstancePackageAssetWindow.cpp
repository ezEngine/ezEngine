#include <EditorPluginSubstance/EditorPluginSubstancePCH.h>

#include <EditorFramework/Assets/AssetStatusIndicator.moc.h>
#include <EditorFramework/DocumentWindow/OrbitCamViewWidget.moc.h>
#include <EditorFramework/InputContexts/EditorInputContext.h>
#include <EditorPluginAssets/TextureAsset/TextureAssetWindow.moc.h>
#include <EditorPluginSubstance/Assets/SubstancePackageAsset.h>
#include <EditorPluginSubstance/Assets/SubstancePackageAssetWindow.moc.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <GuiFoundation/DockPanels/DocumentPanel.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>

ezQtSubstancePackageAssetWindow::ezQtSubstancePackageAssetWindow(ezSubstancePackageAssetDocument* pDocument)
  : ezQtEngineDocumentWindow(pDocument)
{
  if (pDocument->m_SelectedOutput.IsValid() == false)
  {
    auto pMetaData = pDocument->GetAssetDocumentInfo()->GetMetaInfo<ezSubstancePackageAssetMetaData>();
    if (pMetaData->m_OutputUuids.GetCount() > 0)
      pDocument->m_SelectedOutput = pMetaData->m_OutputUuids[0];
  }

  // Menu Bar
  {
    ezQtMenuBarActionMapView* pMenuBar = static_cast<ezQtMenuBarActionMapView*>(menuBar());
    ezActionContext context;
    context.m_sMapping = "SubstanceAssetMenuBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pMenuBar->SetActionContext(context);
  }

  // Tool Bar
  {
    ezQtToolBarActionMapView* pToolBar = new ezQtToolBarActionMapView("Toolbar", this);
    ezActionContext context;
    context.m_sMapping = "SubstanceAssetToolBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pToolBar->SetActionContext(context);
    pToolBar->setObjectName("SubstanceAssetWindowToolBar");
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
    pPropertyPanel->setObjectName("SubstanceAssetDockWidget");
    pPropertyPanel->setWindowTitle("Substance Package Properties");
    pPropertyPanel->show();

    ezQtPropertyGridWidget* pPropertyGrid = new ezQtPropertyGridWidget(pPropertyPanel, pDocument);

    QWidget* pWidget = new QWidget();
    pWidget->setObjectName("Group");
    pWidget->setLayout(new QVBoxLayout());
    pWidget->setContentsMargins(0, 0, 0, 0);

    pWidget->layout()->setContentsMargins(0, 0, 0, 0);
    pWidget->layout()->addWidget(new ezQtAssetStatusIndicator((ezAssetDocument*)GetDocument()));
    pWidget->layout()->addWidget(pPropertyGrid);

    pPropertyPanel->setWidget(pWidget, ads::CDockWidget::ForceNoScrollArea);

    m_pDockManager->addDockWidgetTab(ads::RightDockWidgetArea, pPropertyPanel);

    pDocument->GetSelectionManager()->SetSelection(pDocument->GetObjectManager()->GetRootObject()->GetChildren()[0]);
  }

  FinishWindowCreation();
}

void ezQtSubstancePackageAssetWindow::InternalRedraw()
{
  ezEditorInputContext::UpdateActiveInputContext();
  SendRedrawMsg();
  ezQtEngineDocumentWindow::InternalRedraw();
}

void ezQtSubstancePackageAssetWindow::SendRedrawMsg()
{
  // do not try to redraw while the process is crashed, it is obviously futile
  if (ezEditorEngineProcessConnection::GetSingleton()->IsProcessCrashed())
    return;

  {
    const ezSubstancePackageAssetDocument* pDoc = static_cast<const ezSubstancePackageAssetDocument*>(GetDocument());

    {
      ezDocumentConfigMsgToEngine msg;
      msg.m_sWhatToDo = "SetChannelMode";
      msg.m_iValue = pDoc->m_ChannelMode.GetValue();
      msg.m_fValue = 0.5f;
      GetEditorEngineConnection()->SendMessage(&msg);
    }

    {
      ezDocumentConfigMsgToEngine msg;
      msg.m_sWhatToDo = "SetLodLevel";
      msg.m_iValue = pDoc->m_iTextureLod;
      GetEditorEngineConnection()->SendMessage(&msg);
    }

    {
      ezStringBuilder tmp;

      ezDocumentConfigMsgToEngine msg;
      msg.m_sWhatToDo = "SetTexture";
      msg.m_sValue = ezConversionUtils::ToString(pDoc->m_SelectedOutput, tmp);
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

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSubstanceSelectOutputAction, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezSubstanceSelectOutputAction::ezSubstanceSelectOutputAction(const ezActionContext& context, const char* szName, const char* szIconPath)
  : ezDynamicMenuAction(context, szName, szIconPath)
{
}

void ezSubstanceSelectOutputAction::GetEntries(ezDynamicArray<Item>& out_entries)
{
  ezSubstancePackageAssetDocument* pDocument = static_cast<ezSubstancePackageAssetDocument*>(m_Context.m_pDocument);
  auto pMetaData = pDocument->GetAssetDocumentInfo()->GetMetaInfo<ezSubstancePackageAssetMetaData>();

  out_entries.Clear();
  out_entries.Reserve(pMetaData->m_OutputNames.GetCount());

  for (ezUInt32 i = 0; i < pMetaData->m_OutputNames.GetCount(); ++i)
  {
    auto& item = out_entries.ExpandAndGetRef();
    item.m_sDisplay = pMetaData->m_OutputNames[i];

    const ezUuid& uuid = pMetaData->m_OutputUuids[i];
    item.m_UserValue = uuid;
    item.m_CheckState = (uuid == pDocument->m_SelectedOutput) ? Item::CheckMark::Checked : Item::CheckMark::Unchecked;
  }
}

void ezSubstanceSelectOutputAction::Execute(const ezVariant& value)
{
  if (value.IsA<ezUuid>())
  {
    ezSubstancePackageAssetDocument* pDocument = static_cast<ezSubstancePackageAssetDocument*>(m_Context.m_pDocument);
    pDocument->m_SelectedOutput = value.Get<ezUuid>();
  }
}

//////////////////////////////////////////////////////////////////////////

ezActionDescriptorHandle ezSubstancePackageAssetActions::s_hSelectedOutput;
ezActionDescriptorHandle ezSubstancePackageAssetActions::s_hTextureChannelMode;
ezActionDescriptorHandle ezSubstancePackageAssetActions::s_hLodSlider;

void ezSubstancePackageAssetActions::RegisterActions()
{
  s_hSelectedOutput = EZ_REGISTER_DYNAMIC_MENU("SubstancePackageAsset.SelectedOutput", ezSubstanceSelectOutputAction, ":/AssetIcons/SubstanceDesigner.svg");
  s_hTextureChannelMode = EZ_REGISTER_DYNAMIC_MENU("SubstancePackageAsset.ChannelMode", ezTextureChannelModeAction, ":/EditorFramework/Icons/RenderMode.svg");
  s_hLodSlider = EZ_REGISTER_ACTION_0("SubstancePackageAsset.LodSlider", ezActionScope::Document, "Texture 2D", "", ezTextureLodSliderAction);
}

void ezSubstancePackageAssetActions::UnregisterActions()
{
  ezActionManager::UnregisterAction(s_hSelectedOutput);
  ezActionManager::UnregisterAction(s_hTextureChannelMode);
  ezActionManager::UnregisterAction(s_hLodSlider);
}

void ezSubstancePackageAssetActions::MapToolbarActions(ezStringView sMapping)
{
  ezActionMap* pMap = ezActionMapManager::GetActionMap(sMapping);
  EZ_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", sMapping);

  pMap->MapAction(s_hSelectedOutput, "", 14.0f);
  pMap->MapAction(s_hLodSlider, "", 15.0f);
  pMap->MapAction(s_hTextureChannelMode, "", 16.0f);
}
