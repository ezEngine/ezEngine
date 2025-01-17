#include <EditorPluginParticle/EditorPluginParticlePCH.h>

#include <EditorFramework/Assets/AssetStatusIndicator.moc.h>
#include <EditorFramework/DocumentWindow/OrbitCamViewWidget.moc.h>
#include <EditorFramework/InputContexts/EditorInputContext.h>
#include <EditorPluginParticle/ParticleEffectAsset/ParticleEffectAssetWindow.moc.h>
#include <Foundation/Utilities/AssetFileHeader.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <GuiFoundation/DockPanels/DocumentPanel.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <ParticlePlugin/Behavior/ParticleBehavior_Gravity.h>
#include <ParticlePlugin/Emitter/ParticleEmitter_Continuous.h>
#include <ParticlePlugin/Initializer/ParticleInitializer_RandomColor.h>
#include <ParticlePlugin/Initializer/ParticleInitializer_VelocityCone.h>
#include <ParticlePlugin/System/ParticleSystemDescriptor.h>
#include <ParticlePlugin/Type/Quad/ParticleTypeQuad.h>
#include <QBoxLayout>
#include <QComboBox>
#include <QInputDialog>
#include <QToolButton>
#include <SharedPluginAssets/Common/Messages.h>
#include <ToolsFoundation/Command/TreeCommands.h>

ezQtParticleEffectAssetDocumentWindow::ezQtParticleEffectAssetDocumentWindow(ezAssetDocument* pDocument)
  : ezQtEngineDocumentWindow(pDocument)
{
  GetDocument()->GetObjectManager()->m_PropertyEvents.AddEventHandler(ezMakeDelegate(&ezQtParticleEffectAssetDocumentWindow::PropertyEventHandler, this));
  GetDocument()->GetObjectManager()->m_StructureEvents.AddEventHandler(ezMakeDelegate(&ezQtParticleEffectAssetDocumentWindow::StructureEventHandler, this));


  // Menu Bar
  {
    ezQtMenuBarActionMapView* pMenuBar = static_cast<ezQtMenuBarActionMapView*>(menuBar());
    ezActionContext context;
    context.m_sMapping = "ParticleEffectAssetMenuBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pMenuBar->SetActionContext(context);
  }

  // Tool Bar
  {
    ezQtToolBarActionMapView* pToolBar = new ezQtToolBarActionMapView("Toolbar", this);
    ezActionContext context;
    context.m_sMapping = "ParticleEffectAssetToolBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pToolBar->SetActionContext(context);
    pToolBar->setObjectName("ParticleEffectAssetWindowToolBar");
    addToolBar(pToolBar);
  }

  ezDocumentObject* pRootObject = pDocument->GetObjectManager()->GetRootObject()->GetChildren()[0];

  // ezQtDocumentPanel* pMainPropertyPanel = new ezQtDocumentPanel(this);
  ezQtDocumentPanel* pEffectPanel = new ezQtDocumentPanel(this, pDocument);
  ezQtDocumentPanel* pReactionsPanel = new ezQtDocumentPanel(this, pDocument);
  ezQtDocumentPanel* pSystemsPanel = new ezQtDocumentPanel(this, pDocument);
  ezQtDocumentPanel* pEmitterPanel = new ezQtDocumentPanel(this, pDocument);
  ezQtDocumentPanel* pInitializerPanel = new ezQtDocumentPanel(this, pDocument);
  ezQtDocumentPanel* pBehaviorPanel = new ezQtDocumentPanel(this, pDocument);
  ezQtDocumentPanel* pTypePanel = new ezQtDocumentPanel(this, pDocument);


  // 3D View
  {
    SetTargetFramerate(25);

    m_ViewConfig.m_Camera.LookAt(ezVec3(-1.6f, 0, 0), ezVec3(0, 0, 0), ezVec3(0, 0, 1));
    m_ViewConfig.ApplyPerspectiveSetting(90);

    m_pViewWidget = new ezQtOrbitCamViewWidget(this, &m_ViewConfig);
    m_pViewWidget->ConfigureRelative(ezVec3(0), ezVec3(5.0f), ezVec3(-2, 0, 0.5f), 1.0f);
    AddViewWidget(m_pViewWidget);
    ezQtViewWidgetContainer* pContainer = new ezQtViewWidgetContainer(this, m_pViewWidget, "ParticleEffectAssetViewToolBar");
    m_pDockManager->setCentralWidget(pContainer);
  }

  // Property Grid
  //{
  //  pMainPropertyPanel->setObjectName("ParticleEffectAssetDockWidget");
  //  pMainPropertyPanel->setWindowTitle("Particle Effect Properties");
  //  pMainPropertyPanel->show();

  //  ezQtPropertyGridWidget* pPropertyGrid = new ezQtPropertyGridWidget(pMainPropertyPanel, pDocument);
  //  pMainPropertyPanel->setWidget(pPropertyGrid);

  //  addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, pMainPropertyPanel);

  //  pDocument->GetSelectionManager()->SetSelection(pRootObject);
  //}

  // Effect Properties
  {
    pEffectPanel->setObjectName("ParticleEffectAssetDockWidget_Effect");
    pEffectPanel->setWindowTitle("Effect");
    pEffectPanel->show();

    ezQtPropertyGridWidget* pPropertyGrid = new ezQtPropertyGridWidget(pEffectPanel, pDocument, false);

    QWidget* pWidget = new QWidget();
    pWidget->setObjectName("Group");
    pWidget->setLayout(new QVBoxLayout());
    pWidget->setContentsMargins(0, 0, 0, 0);

    pWidget->layout()->setContentsMargins(0, 0, 0, 0);
    pWidget->layout()->addWidget(new ezQtAssetStatusIndicator((ezAssetDocument*)GetDocument()));
    pWidget->layout()->addWidget(pPropertyGrid);

    pEffectPanel->setWidget(pWidget, ads::CDockWidget::ForceNoScrollArea);

    ezDeque<const ezDocumentObject*> sel;
    sel.PushBack(pRootObject);
    pPropertyGrid->SetSelectionIncludeExcludeProperties(nullptr, "EventReactions;ParticleSystems");
    pPropertyGrid->SetSelection(sel);

    m_pDockManager->addDockWidget(ads::RightDockWidgetArea, pEffectPanel);
  }

  // Particle Systems Panel
  {
    pSystemsPanel->setObjectName("ParticleEffectAssetDockWidget_Systems");
    pSystemsPanel->setWindowTitle("Systems");
    pSystemsPanel->show();

    QWidget* pMainWidget = new QFrame(pSystemsPanel);
    pMainWidget->setContentsMargins(0, 0, 0, 0);
    pMainWidget->setLayout(new QVBoxLayout(pMainWidget));
    pMainWidget->layout()->setContentsMargins(0, 0, 0, 0);

    {
      QWidget* pGroup = new QWidget(pMainWidget);
      pGroup->setContentsMargins(0, 0, 0, 0);
      pGroup->setLayout(new QHBoxLayout(pGroup));
      pGroup->layout()->setContentsMargins(0, 0, 0, 0);

      m_pSystemsCombo = new QComboBox(pSystemsPanel);
      connect(m_pSystemsCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(onSystemSelected(int)));

      m_pAddSystem = new QToolButton(pSystemsPanel);
      connect(m_pAddSystem, &QAbstractButton::clicked, this, &ezQtParticleEffectAssetDocumentWindow::onAddSystem);

      m_pRemoveSystem = new QToolButton(pSystemsPanel);
      connect(m_pRemoveSystem, &QAbstractButton::clicked, this, &ezQtParticleEffectAssetDocumentWindow::onRemoveSystem);

      m_pRenameSystem = new QToolButton(pSystemsPanel);
      connect(m_pRenameSystem, &QAbstractButton::clicked, this, &ezQtParticleEffectAssetDocumentWindow::onRenameSystem);

      m_pAddSystem->setToolTip("Add another particle system to the effect.");
      m_pRemoveSystem->setToolTip("Remove this particle system from the effect.");
      m_pRenameSystem->setToolTip("Rename this particle system.");

      m_pAddSystem->setIcon(QIcon(":/GuiFoundation/Icons/Add.svg"));
      m_pRemoveSystem->setIcon(QIcon(":/GuiFoundation/Icons/Delete.svg"));
      m_pRenameSystem->setIcon(QIcon(":/GuiFoundation/Icons/Rename.svg"));

      pGroup->layout()->addWidget(m_pRenameSystem);
      pGroup->layout()->addWidget(m_pSystemsCombo);
      pGroup->layout()->addWidget(m_pAddSystem);
      pGroup->layout()->addWidget(m_pRemoveSystem);

      pMainWidget->layout()->addWidget(pGroup);
    }

    m_pPropertyGridSystems = new ezQtPropertyGridWidget(pSystemsPanel, pDocument);
    m_pPropertyGridSystems->SetSelectionIncludeExcludeProperties(nullptr, "Name;Emitters;Initializers;Behaviors;Types");
    pMainWidget->layout()->addWidget(m_pPropertyGridSystems);

    if (!pRootObject->GetChildren().IsEmpty())
    {
      ezDeque<const ezDocumentObject*> sel;
      sel.PushBack(pRootObject->GetChildren()[0]);
      m_pPropertyGridSystems->SetSelection(sel);
    }

    pSystemsPanel->setWidget(pMainWidget);

    m_pDockManager->addDockWidget(ads::CenterDockWidgetArea, pSystemsPanel, pEffectPanel->dockAreaWidget());
  }

  // Event Reactions
  {
    pReactionsPanel->setObjectName("ParticleEffectAssetDockWidget_Reactions");
    pReactionsPanel->setWindowTitle("Event Reactions");
    pReactionsPanel->show();

    ezQtPropertyGridWidget* pPropertyGrid = new ezQtPropertyGridWidget(pReactionsPanel, pDocument, false);
    pReactionsPanel->setWidget(pPropertyGrid);

    ezDeque<const ezDocumentObject*> sel;
    sel.PushBack(pRootObject);
    pPropertyGrid->SetSelectionIncludeExcludeProperties("EventReactions");
    pPropertyGrid->SetSelection(sel);

    m_pDockManager->addDockWidget(ads::CenterDockWidgetArea, pReactionsPanel, pEffectPanel->dockAreaWidget());
  }

  // System Emitters
  {
    pEmitterPanel->setObjectName("ParticleEffectAssetDockWidget_Emitter");
    pEmitterPanel->setWindowTitle("Emitter");
    pEmitterPanel->show();

    m_pPropertyGridEmitter = new ezQtPropertyGridWidget(pEmitterPanel, pDocument, false);
    m_pPropertyGridEmitter->SetSelectionIncludeExcludeProperties("Emitters");
    pEmitterPanel->setWidget(m_pPropertyGridEmitter);

    m_pDockManager->addDockWidget(ads::BottomDockWidgetArea, pEmitterPanel, pEffectPanel->dockAreaWidget());
  }

  // System Initializers
  {
    pInitializerPanel->setObjectName("ParticleEffectAssetDockWidget_Initializer");
    pInitializerPanel->setWindowTitle("Initializers");
    pInitializerPanel->show();

    m_pPropertyGridInitializer = new ezQtPropertyGridWidget(pInitializerPanel, pDocument, false);
    m_pPropertyGridInitializer->SetSelectionIncludeExcludeProperties("Initializers");
    pInitializerPanel->setWidget(m_pPropertyGridInitializer);

    m_pDockManager->addDockWidget(ads::CenterDockWidgetArea, pInitializerPanel, pEmitterPanel->dockAreaWidget());
  }

  // System Behaviors
  {
    pBehaviorPanel->setObjectName("ParticleEffectAssetDockWidget_Behavior");
    pBehaviorPanel->setWindowTitle("Behaviors");
    pBehaviorPanel->show();

    m_pPropertyGridBehavior = new ezQtPropertyGridWidget(pBehaviorPanel, pDocument, false);
    m_pPropertyGridBehavior->SetSelectionIncludeExcludeProperties("Behaviors");
    pBehaviorPanel->setWidget(m_pPropertyGridBehavior);

    m_pDockManager->addDockWidget(ads::CenterDockWidgetArea, pBehaviorPanel, pEmitterPanel->dockAreaWidget());
  }

  // System Types
  {
    pTypePanel->setObjectName("ParticleEffectAssetDockWidget_Type");
    pTypePanel->setWindowTitle("Renderers");
    pTypePanel->show();

    m_pPropertyGridType = new ezQtPropertyGridWidget(pTypePanel, pDocument, false);
    m_pPropertyGridType->SetSelectionIncludeExcludeProperties("Types");
    pTypePanel->setWidget(m_pPropertyGridType);

    m_pDockManager->addDockWidget(ads::CenterDockWidgetArea, pTypePanel, pEmitterPanel->dockAreaWidget());
  }

  m_pAssetDoc = static_cast<ezParticleEffectAssetDocument*>(pDocument);

  pSystemsPanel->raise();
  pEmitterPanel->raise();

  FinishWindowCreation();

  UpdateSystemList();

  GetParticleDocument()->m_Events.AddEventHandler(ezMakeDelegate(&ezQtParticleEffectAssetDocumentWindow::ParticleEventHandler, this));
}

ezQtParticleEffectAssetDocumentWindow::~ezQtParticleEffectAssetDocumentWindow()
{
  GetParticleDocument()->m_Events.RemoveEventHandler(ezMakeDelegate(&ezQtParticleEffectAssetDocumentWindow::ParticleEventHandler, this));

  RestoreResource();

  GetDocument()->GetObjectManager()->m_StructureEvents.RemoveEventHandler(ezMakeDelegate(&ezQtParticleEffectAssetDocumentWindow::StructureEventHandler, this));
  GetDocument()->GetObjectManager()->m_PropertyEvents.RemoveEventHandler(ezMakeDelegate(&ezQtParticleEffectAssetDocumentWindow::PropertyEventHandler, this));
}

const char* ezQtParticleEffectAssetDocumentWindow::GetWindowLayoutGroupName() const
{
  return "ParticleEffectAsset2";
}

ezParticleEffectAssetDocument* ezQtParticleEffectAssetDocumentWindow::GetParticleDocument()
{
  return static_cast<ezParticleEffectAssetDocument*>(GetDocument());
}

void ezQtParticleEffectAssetDocumentWindow::SelectSystem(const ezDocumentObject* pObject)
{
  if (pObject == nullptr)
  {
    m_sSelectedSystem.Clear();

    m_pPropertyGridSystems->ClearSelection();
    m_pPropertyGridEmitter->ClearSelection();
    m_pPropertyGridInitializer->ClearSelection();
    m_pPropertyGridBehavior->ClearSelection();
    m_pPropertyGridType->ClearSelection();

    if (m_pSystemsCombo->currentIndex() != -1)
    {
      // prevent infinite recursion
      m_pSystemsCombo->setCurrentIndex(-1);
    }
  }
  else
  {
    m_sSelectedSystem = pObject->GetTypeAccessor().GetValue("Name").ConvertTo<ezString>();

    ezDeque<const ezDocumentObject*> sel;
    sel.PushBack(pObject);
    GetDocument()->GetSelectionManager()->SetSelection(pObject);
    m_pPropertyGridSystems->SetSelection(sel);

    m_pPropertyGridEmitter->SetSelection(sel);
    m_pPropertyGridInitializer->SetSelection(sel);
    m_pPropertyGridBehavior->SetSelection(sel);
    m_pPropertyGridType->SetSelection(sel);

    m_pSystemsCombo->setCurrentText(m_sSelectedSystem.GetData());
  }
}

void ezQtParticleEffectAssetDocumentWindow::onSystemSelected(int index)
{
  if (index >= 0)
  {
    ezDocumentObject* pObject = static_cast<ezDocumentObject*>(m_pSystemsCombo->itemData(index).value<void*>());

    SelectSystem(pObject);
  }
  else
  {
    SelectSystem(nullptr);
  }
}

ezStatus ezQtParticleEffectAssetDocumentWindow::SetupSystem(ezStringView sName)
{
  const ezDocumentObject* pRootObject = GetParticleDocument()->GetObjectManager()->GetRootObject()->GetChildren()[0];
  ezObjectAccessorBase* pAccessor = GetDocument()->GetObjectAccessor();

  ezUuid systemGuid = ezUuid::MakeUuid();

  EZ_SUCCEED_OR_RETURN(pAccessor->AddObjectByName(pRootObject, "ParticleSystems", -1, ezGetStaticRTTI<ezParticleSystemDescriptor>(), systemGuid));

  const ezDocumentObject* pSystemObject = pAccessor->GetObject(systemGuid);

  EZ_SUCCEED_OR_RETURN(pAccessor->SetValueByName(pSystemObject, "Name", sName));

  // default system setup
  {
    {
      ezVarianceTypeTime val;
      val.m_Value = ezTime::MakeFromSeconds(1.0f);
      EZ_SUCCEED_OR_RETURN(pAccessor->SetValueByName(pSystemObject, "LifeTime", val));
    }

    // add emitter
    {
      ezUuid emitterGuid = ezUuid::MakeUuid();
      EZ_SUCCEED_OR_RETURN(pAccessor->AddObjectByName(pSystemObject, "Emitters", -1, ezGetStaticRTTI<ezParticleEmitterFactory_Continuous>(), emitterGuid));
    }

    // add cone velocity initializer
    {
      ezUuid velocityGuid = ezUuid::MakeUuid();
      EZ_SUCCEED_OR_RETURN(pAccessor->AddObjectByName(pSystemObject, "Initializers", -1, ezGetStaticRTTI<ezParticleInitializerFactory_VelocityCone>(), velocityGuid));

      const ezDocumentObject* pConeObject = pAccessor->GetObject(velocityGuid);

      // default speed
      {
        ezVarianceTypeFloat val;
        val.m_Value = 4.0f;
        EZ_SUCCEED_OR_RETURN(pAccessor->SetValueByName(pConeObject, "Speed", val));
      }
    }

    // add color initializer
    {
      ezUuid colorInitGuid = ezUuid::MakeUuid();
      EZ_SUCCEED_OR_RETURN(pAccessor->AddObjectByName(pSystemObject, "Initializers", -1, ezGetStaticRTTI<ezParticleInitializerFactory_RandomColor>(), colorInitGuid));

      const ezDocumentObject* pColorObject = pAccessor->GetObject(colorInitGuid);

      EZ_SUCCEED_OR_RETURN(pAccessor->SetValueByName(pColorObject, "Color1", ezColor::Red));
      EZ_SUCCEED_OR_RETURN(pAccessor->SetValueByName(pColorObject, "Color2", ezColor::Yellow));
    }

    // add gravity behavior
    {
      ezUuid gravityGuid = ezUuid::MakeUuid();
      EZ_SUCCEED_OR_RETURN(pAccessor->AddObjectByName(pSystemObject, "Behaviors", -1, ezGetStaticRTTI<ezParticleBehaviorFactory_Gravity>(), gravityGuid));
    }

    // add quad renderer
    {
      ezUuid quadGuid = ezUuid::MakeUuid();
      EZ_SUCCEED_OR_RETURN(pAccessor->AddObjectByName(pSystemObject, "Types", -1, ezGetStaticRTTI<ezParticleTypeQuadFactory>(), quadGuid));
    }
  }

  m_sSelectedSystem = sName;
  UpdateSystemList();
  SelectSystem(pSystemObject);

  return ezStatus(EZ_SUCCESS);
}

void ezQtParticleEffectAssetDocumentWindow::onAddSystem(bool)
{
  bool ok = false;
  QString sName;

  while (true)
  {
    sName = QInputDialog::getText(this, "New Particle System", "Name:", QLineEdit::Normal, QString(), &ok);

    if (!ok)
      return;

    if (sName.isEmpty())
    {
      ezQtUiServices::GetSingleton()->MessageBoxInformation("Invalid particle system name.");
      continue;
    }

    if (m_ParticleSystems.Find(sName.toUtf8().data()).IsValid())
    {
      ezQtUiServices::GetSingleton()->MessageBoxInformation("A particle system with this name exists already.");
      continue;
    }

    break;
  }

  ezObjectAccessorBase* pAccessor = GetDocument()->GetObjectAccessor();

  pAccessor->StartTransaction("Add Particle System");

  if (SetupSystem(qtToEzString(sName)).Failed())
  {
    pAccessor->CancelTransaction();
  }
  else
  {
    pAccessor->FinishTransaction();
  }

  m_bDoLiveResourceUpdate = true;
}

void ezQtParticleEffectAssetDocumentWindow::onRemoveSystem(bool)
{
  const int index = m_pSystemsCombo->findText(m_sSelectedSystem.GetData());
  if (index < 0)
    return;

  const ezDocumentObject* pObject = static_cast<ezDocumentObject*>(m_pSystemsCombo->itemData(index).value<void*>());

  GetDocument()->GetObjectAccessor()->StartTransaction("Rename Particle System");

  ezRemoveObjectCommand cmd;
  cmd.m_Object = pObject->GetGuid();

  if (GetDocument()->GetCommandHistory()->AddCommand(cmd).Failed())
  {
    GetDocument()->GetObjectAccessor()->CancelTransaction();
    return;
  }

  GetDocument()->GetObjectAccessor()->FinishTransaction();

  UpdateSystemList();
}

void ezQtParticleEffectAssetDocumentWindow::onRenameSystem(bool)
{
  const int index = m_pSystemsCombo->findText(m_sSelectedSystem.GetData());
  if (index < 0)
    return;

  const ezDocumentObject* pObject = static_cast<ezDocumentObject*>(m_pSystemsCombo->itemData(index).value<void*>());

  bool ok = false;
  const QString sOrgName = pObject->GetTypeAccessor().GetValue("Name").ConvertTo<ezString>().GetData();
  QString sName;

  while (true)
  {
    sName = QInputDialog::getText(this, "Rename Particle System", "Name:", QLineEdit::Normal, sOrgName, &ok);

    if (!ok || sName == sOrgName)
      return;

    if (sName.isEmpty())
    {
      ezQtUiServices::GetSingleton()->MessageBoxInformation("Invalid particle system name.");
      continue;
    }

    if (m_ParticleSystems.Find(sName.toUtf8().data()).IsValid())
    {
      ezQtUiServices::GetSingleton()->MessageBoxInformation("A particle system with this name exists already.");
      continue;
    }

    break;
  }

  m_sSelectedSystem = sName.toUtf8().data();

  GetDocument()->GetObjectAccessor()->StartTransaction("Rename Particle System");

  ezSetObjectPropertyCommand cmd2;
  cmd2.m_Object = pObject->GetGuid();
  cmd2.m_NewValue = sName.toUtf8().data();
  cmd2.m_sProperty = "Name";
  cmd2.m_Index = 0;

  if (GetDocument()->GetCommandHistory()->AddCommand(cmd2).Failed())
  {
    GetDocument()->GetObjectAccessor()->CancelTransaction();
    return;
  }

  GetDocument()->GetObjectAccessor()->FinishTransaction();

  UpdateSystemList();
}

void ezQtParticleEffectAssetDocumentWindow::SendLiveResourcePreview()
{
  if (ezEditorEngineProcessConnection::GetSingleton()->IsProcessCrashed())
    return;

  ezResourceUpdateMsgToEngine msg;
  msg.m_sResourceType = "Particle Effect";

  ezStringBuilder tmp;
  msg.m_sResourceID = ezConversionUtils::ToString(GetDocument()->GetGuid(), tmp);

  ezContiguousMemoryStreamStorage streamStorage;
  ezMemoryStreamWriter memoryWriter(&streamStorage);

  // Write Path
  ezStringBuilder sAbsFilePath = GetParticleDocument()->GetDocumentPath();
  sAbsFilePath.ChangeFileExtension("ezParticleEffect");

  // Write Header
  memoryWriter << sAbsFilePath;
  const ezUInt64 uiHash = ezAssetCurator::GetSingleton()->GetAssetDependencyHash(GetParticleDocument()->GetGuid());
  ezAssetFileHeader AssetHeader;
  AssetHeader.SetFileHashAndVersion(uiHash, GetParticleDocument()->GetAssetTypeVersion());
  AssetHeader.Write(memoryWriter).IgnoreResult();

  // Write Asset Data
  GetParticleDocument()->WriteResource(memoryWriter);
  msg.m_Data = ezArrayPtr<const ezUInt8>(streamStorage.GetData(), streamStorage.GetStorageSize32());

  ezEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
}

void ezQtParticleEffectAssetDocumentWindow::PropertyEventHandler(const ezDocumentObjectPropertyEvent& e)
{
  m_bDoLiveResourceUpdate = true;
}

void ezQtParticleEffectAssetDocumentWindow::StructureEventHandler(const ezDocumentObjectStructureEvent& e)
{
  switch (e.m_EventType)
  {
    case ezDocumentObjectStructureEvent::Type::AfterObjectAdded:
    case ezDocumentObjectStructureEvent::Type::AfterObjectMoved2:
    case ezDocumentObjectStructureEvent::Type::AfterObjectRemoved:
      m_bDoLiveResourceUpdate = true;
      break;

    default:
      break;
  }
}


void ezQtParticleEffectAssetDocumentWindow::ParticleEventHandler(const ezParticleEffectAssetEvent& e)
{
  switch (e.m_Type)
  {
    case ezParticleEffectAssetEvent::RestartEffect:
    {
      ezEditorEngineRestartSimulationMsg msg;
      GetEditorEngineConnection()->SendMessage(&msg);
    }
    break;

    case ezParticleEffectAssetEvent::AutoRestartChanged:
    {
      ezEditorEngineLoopAnimationMsg msg;
      msg.m_bLoop = GetParticleDocument()->GetAutoRestart();
      GetEditorEngineConnection()->SendMessage(&msg);
    }
    break;

    default:
      break;
  }
}

void ezQtParticleEffectAssetDocumentWindow::UpdateSystemList()
{
  ezMap<ezString, ezDocumentObject*> newParticleSystems;

  ezDocumentObject* pRootObject = GetParticleDocument()->GetObjectManager()->GetRootObject()->GetChildren()[0];

  ezStringBuilder s;

  for (ezDocumentObject* pChild : pRootObject->GetChildren())
  {
    if (pChild->GetParentProperty() == "ParticleSystems"_ezsv)
    {
      s = pChild->GetTypeAccessor().GetValue("Name").ConvertTo<ezString>();
      newParticleSystems[s] = pChild;
    }
  }

  // early out
  if (m_ParticleSystems == newParticleSystems)
    return;

  m_ParticleSystems.Swap(newParticleSystems);

  {
    ezQtScopedBlockSignals _1(m_pSystemsCombo);
    m_pSystemsCombo->clear();

    for (auto it = m_ParticleSystems.GetIterator(); it.IsValid(); ++it)
    {
      m_pSystemsCombo->addItem(it.Key().GetData(), QVariant::fromValue<void*>(it.Value()));
    }
  }

  if (!m_ParticleSystems.Find(m_sSelectedSystem).IsValid())
    m_sSelectedSystem.Clear();

  if (m_sSelectedSystem.IsEmpty() && !m_ParticleSystems.IsEmpty())
    m_sSelectedSystem = m_ParticleSystems.GetIterator().Key();

  if (!m_ParticleSystems.IsEmpty())
  {
    SelectSystem(m_ParticleSystems[m_sSelectedSystem]);
  }
  else
  {
    SelectSystem(nullptr);
  }

  const bool hasSelection = !m_ParticleSystems.IsEmpty();

  m_pSystemsCombo->setEnabled(hasSelection);
  m_pRemoveSystem->setEnabled(hasSelection);
  m_pRenameSystem->setEnabled(hasSelection);
}


void ezQtParticleEffectAssetDocumentWindow::InternalRedraw()
{
  ezEditorInputContext::UpdateActiveInputContext();
  SendRedrawMsg();
  ezQtEngineDocumentWindow::InternalRedraw();
}


void ezQtParticleEffectAssetDocumentWindow::SendRedrawMsg()
{
  // do not try to redraw while the process is crashed, it is obviously futile
  if (ezEditorEngineProcessConnection::GetSingleton()->IsProcessCrashed())
    return;

  if (m_bDoLiveResourceUpdate)
  {
    SendLiveResourcePreview();
    m_bDoLiveResourceUpdate = false;
  }

  {
    ezSimulationSettingsMsgToEngine msg;
    msg.m_bSimulateWorld = !GetParticleDocument()->GetSimulationPaused();
    msg.m_fSimulationSpeed = GetParticleDocument()->GetSimulationSpeed();
    GetEditorEngineConnection()->SendMessage(&msg);
  }

  for (auto pView : m_ViewWidgets)
  {
    pView->SetEnablePicking(false);
    pView->UpdateCameraInterpolation();
    pView->SyncToEngine();
  }
}

void ezQtParticleEffectAssetDocumentWindow::RestoreResource()
{
  ezRestoreResourceMsgToEngine msg;
  msg.m_sResourceType = "Particle Effect";

  ezStringBuilder tmp;
  msg.m_sResourceID = ezConversionUtils::ToString(GetDocument()->GetGuid(), tmp);

  ezEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
}
