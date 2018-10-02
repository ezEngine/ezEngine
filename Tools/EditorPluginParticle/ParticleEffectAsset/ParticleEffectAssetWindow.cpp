#include <PCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/DocumentWindow/OrbitCamViewWidget.moc.h>
#include <EditorFramework/InputContexts/EditorInputContext.h>
#include <EditorPluginParticle/ParticleEffectAsset/ParticleEffectAssetWindow.moc.h>
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
  GetDocument()->GetObjectManager()->m_PropertyEvents.AddEventHandler(
      ezMakeDelegate(&ezQtParticleEffectAssetDocumentWindow::PropertyEventHandler, this));
  GetDocument()->GetObjectManager()->m_StructureEvents.AddEventHandler(
      ezMakeDelegate(&ezQtParticleEffectAssetDocumentWindow::StructureEventHandler, this));


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
  ezQtDocumentPanel* pEffectPanel = new ezQtDocumentPanel(this);
  ezQtDocumentPanel* pReactionsPanel = new ezQtDocumentPanel(this);
  ezQtDocumentPanel* pSystemsPanel = new ezQtDocumentPanel(this);
  ezQtDocumentPanel* pEmitterPanel = new ezQtDocumentPanel(this);
  ezQtDocumentPanel* pInitializerPanel = new ezQtDocumentPanel(this);
  ezQtDocumentPanel* pBehaviorPanel = new ezQtDocumentPanel(this);
  ezQtDocumentPanel* pTypePanel = new ezQtDocumentPanel(this);

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

  // Particle Systems Panel
  {
    pSystemsPanel->setObjectName("ParticleEffectAssetDockWidget_Systems");
    pSystemsPanel->setWindowTitle("Systems");
    pSystemsPanel->show();

    QWidget* pMainWidget = new QWidget(pSystemsPanel);
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

      m_pAddSystem->setIcon(QIcon(":/GuiFoundation/Icons/Add16.png"));
      m_pRemoveSystem->setIcon(QIcon(":/GuiFoundation/Icons/Delete16.png"));
      m_pRenameSystem->setIcon(QIcon(":/GuiFoundation/Icons/Rename16.png"));

      pGroup->layout()->addWidget(m_pRenameSystem);
      pGroup->layout()->addWidget(m_pSystemsCombo);
      pGroup->layout()->addWidget(m_pAddSystem);
      pGroup->layout()->addWidget(m_pRemoveSystem);

      pMainWidget->layout()->addWidget(pGroup);
    }

    m_pPropertyGridSystems = new ezQtPropertyGridWidget(pSystemsPanel, pDocument);
    pMainWidget->layout()->addWidget(m_pPropertyGridSystems);

    {
      ezDeque<const ezDocumentObject*> sel;
      sel.PushBack(pRootObject->GetChildren()[0]);
      m_pPropertyGridSystems->SetSelection(sel, nullptr, "Name;Emitters;Initializers;Behaviors;Types");
    }

    pMainWidget->layout()->addItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::MinimumExpanding));
    pSystemsPanel->setWidget(pMainWidget);

    addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, pSystemsPanel);
  }

  // Effect Properties
  {
    pEffectPanel->setObjectName("ParticleEffectAssetDockWidget_Effect");
    pEffectPanel->setWindowTitle("Effect");
    pEffectPanel->show();

    ezQtPropertyGridWidget* pPropertyGrid = new ezQtPropertyGridWidget(pEffectPanel, pDocument, false);
    pEffectPanel->setWidget(pPropertyGrid);

    addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, pEffectPanel);

    ezDeque<const ezDocumentObject*> sel;
    sel.PushBack(pRootObject);
    pPropertyGrid->SetSelection(sel, nullptr, "EventReactions;ParticleSystems");
  }

  // Event Reactions
  {
    pReactionsPanel->setObjectName("ParticleEffectAssetDockWidget_Reactions");
    pReactionsPanel->setWindowTitle("Event Reactions");
    pReactionsPanel->show();

    ezQtPropertyGridWidget* pPropertyGrid = new ezQtPropertyGridWidget(pReactionsPanel, pDocument, false);
    pReactionsPanel->setWidget(pPropertyGrid);

    addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, pReactionsPanel);

    ezDeque<const ezDocumentObject*> sel;
    sel.PushBack(pRootObject);
    pPropertyGrid->SetSelection(sel, "EventReactions");
  }

  // System Emitters
  {
    pEmitterPanel->setObjectName("ParticleEffectAssetDockWidget_Emitter");
    pEmitterPanel->setWindowTitle("Emitter");
    pEmitterPanel->show();

    m_pPropertyGridEmitter = new ezQtPropertyGridWidget(pEmitterPanel, pDocument, false);
    pEmitterPanel->setWidget(m_pPropertyGridEmitter);

    addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, pEmitterPanel);
  }

  // System Initializers
  {
    pInitializerPanel->setObjectName("ParticleEffectAssetDockWidget_Initializer");
    pInitializerPanel->setWindowTitle("Initializers");
    pInitializerPanel->show();

    m_pPropertyGridInitializer = new ezQtPropertyGridWidget(pInitializerPanel, pDocument, false);
    pInitializerPanel->setWidget(m_pPropertyGridInitializer);

    addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, pInitializerPanel);
  }

  // System Behaviors
  {
    pBehaviorPanel->setObjectName("ParticleEffectAssetDockWidget_Behavior");
    pBehaviorPanel->setWindowTitle("Behaviors");
    pBehaviorPanel->show();

    m_pPropertyGridBehavior = new ezQtPropertyGridWidget(pBehaviorPanel, pDocument, false);
    pBehaviorPanel->setWidget(m_pPropertyGridBehavior);

    addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, pBehaviorPanel);
  }

  // System Types
  {
    pTypePanel->setObjectName("ParticleEffectAssetDockWidget_Type");
    pTypePanel->setWindowTitle("Renderers");
    pTypePanel->show();

    m_pPropertyGridType = new ezQtPropertyGridWidget(pTypePanel, pDocument, false);
    pTypePanel->setWidget(m_pPropertyGridType);

    addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, pTypePanel);
  }

  // 3D View
  {
    SetTargetFramerate(25);

    m_ViewConfig.m_Camera.LookAt(ezVec3(-1.6, 0, 0), ezVec3(0, 0, 0), ezVec3(0, 0, 1));
    m_ViewConfig.ApplyPerspectiveSetting(90);

    m_pViewWidget = new ezQtOrbitCamViewWidget(this, &m_ViewConfig);
    m_pViewWidget->ConfigureOrbitCameraVolume(ezVec3(0), ezVec3(5.0f), ezVec3(-2, 0, 0.5f));
    AddViewWidget(m_pViewWidget);
    ezQtViewWidgetContainer* pContainer = new ezQtViewWidgetContainer(this, m_pViewWidget, "ParticleEffectAssetViewToolBar");
    setCentralWidget(pContainer);
  }

  m_pAssetDoc = static_cast<ezParticleEffectAssetDocument*>(pDocument);

  tabifyDockWidget(pEffectPanel, pSystemsPanel);
  tabifyDockWidget(pEffectPanel, pReactionsPanel);

  tabifyDockWidget(pEmitterPanel, pInitializerPanel);
  tabifyDockWidget(pEmitterPanel, pBehaviorPanel);
  tabifyDockWidget(pEmitterPanel, pTypePanel);

  pSystemsPanel->raise();
  pEmitterPanel->raise();

  FinishWindowCreation();

  UpdateSystemList();
  UpdatePreview();

  GetParticleDocument()->m_Events.AddEventHandler(ezMakeDelegate(&ezQtParticleEffectAssetDocumentWindow::ParticleEventHandler, this));
}

ezQtParticleEffectAssetDocumentWindow::~ezQtParticleEffectAssetDocumentWindow()
{
  GetParticleDocument()->m_Events.RemoveEventHandler(ezMakeDelegate(&ezQtParticleEffectAssetDocumentWindow::ParticleEventHandler, this));

  RestoreResource();

  GetDocument()->GetObjectManager()->m_StructureEvents.RemoveEventHandler(
      ezMakeDelegate(&ezQtParticleEffectAssetDocumentWindow::StructureEventHandler, this));
  GetDocument()->GetObjectManager()->m_PropertyEvents.RemoveEventHandler(
      ezMakeDelegate(&ezQtParticleEffectAssetDocumentWindow::PropertyEventHandler, this));
}

const char* ezQtParticleEffectAssetDocumentWindow::GetWindowLayoutGroupName() const
{
  return "ParticleEffectAsset2";
}

ezParticleEffectAssetDocument* ezQtParticleEffectAssetDocumentWindow::GetParticleDocument()
{
  return static_cast<ezParticleEffectAssetDocument*>(GetDocument());
}

void ezQtParticleEffectAssetDocumentWindow::SelectSystem(ezDocumentObject* pObject)
{
  if (pObject == nullptr)
  {
    m_sSelectedSystem.Clear();

    m_pPropertyGridSystems->ClearSelection();
    m_pPropertyGridEmitter->ClearSelection();
    m_pPropertyGridInitializer->ClearSelection();
    m_pPropertyGridBehavior->ClearSelection();
    m_pPropertyGridType->ClearSelection();
  }
  else
  {
    m_sSelectedSystem = pObject->GetTypeAccessor().GetValue("Name").ConvertTo<ezString>();

    ezDeque<const ezDocumentObject*> sel;
    sel.PushBack(pObject);
    m_pPropertyGridSystems->SetSelection(sel, nullptr, "Name;Emitters;Initializers;Behaviors;Types");

    m_pPropertyGridEmitter->SetSelection(sel, "Emitters");
    m_pPropertyGridInitializer->SetSelection(sel, "Initializers");
    m_pPropertyGridBehavior->SetSelection(sel, "Behaviors");
    m_pPropertyGridType->SetSelection(sel, "Types");
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

  ezDocumentObject* pRootObject = GetParticleDocument()->GetObjectManager()->GetRootObject()->GetChildren()[0];

  GetDocument()->GetObjectAccessor()->StartTransaction("Add Particle System");
  ezUuid systemGuid;
  systemGuid.CreateNewUuid();

  {
    ezAddObjectCommand cmd;
    cmd.m_Parent = pRootObject->GetGuid();
    cmd.m_Index = -1;
    cmd.m_pType = ezGetStaticRTTI<ezParticleSystemDescriptor>();
    cmd.m_NewObjectGuid = systemGuid;
    cmd.m_sParentProperty = "ParticleSystems";

    if (GetDocument()->GetCommandHistory()->AddCommand(cmd).Failed())
    {
      GetDocument()->GetObjectAccessor()->CancelTransaction();
      return;
    }
  }

  m_sSelectedSystem = sName.toUtf8().data();

  {
    ezSetObjectPropertyCommand cmd;
    cmd.m_Object = systemGuid;
    cmd.m_NewValue = sName.toUtf8().data();
    cmd.m_sProperty = "Name";
    cmd.m_Index = 0;

    if (GetDocument()->GetCommandHistory()->AddCommand(cmd).Failed())
    {
      GetDocument()->GetObjectAccessor()->CancelTransaction();
      return;
    }
  }

  // default system setup
  {
    const ezDocumentObject* pSystemObject = GetDocument()->GetObjectAccessor()->GetObject(systemGuid);

    // default life
    {
      const ezHybridArray<ezDocumentObject*, 8>& children = pSystemObject->GetChildren();

      for (auto pChild : children)
      {
        if (ezStringUtils::IsEqual(pChild->GetParentProperty(), "LifeTime"))
        {
          ezSetObjectPropertyCommand cmd;
          cmd.m_Object = pChild->GetGuid();
          cmd.m_NewValue = ezTime::Seconds(1);
          cmd.m_sProperty = "Value";
          cmd.m_Index = 0;

          if (GetDocument()->GetCommandHistory()->AddCommand(cmd).Failed())
          {
            GetDocument()->GetObjectAccessor()->CancelTransaction();
            return;
          }

          break;
        }
      }
    }

    // add emitter
    {
      ezAddObjectCommand cmd;
      cmd.m_Parent = systemGuid;
      cmd.m_Index = -1;
      cmd.m_pType = ezGetStaticRTTI<ezParticleEmitterFactory_Continuous>();
      cmd.m_sParentProperty = "Emitters";

      if (GetDocument()->GetCommandHistory()->AddCommand(cmd).Failed())
      {
        GetDocument()->GetObjectAccessor()->CancelTransaction();
        return;
      }
    }

    // add cone velocity initializer
    {
      ezAddObjectCommand cmd;
      cmd.m_Parent = systemGuid;
      cmd.m_Index = -1;
      cmd.m_pType = ezGetStaticRTTI<ezParticleInitializerFactory_VelocityCone>();
      cmd.m_sParentProperty = "Initializers";

      if (GetDocument()->GetCommandHistory()->AddCommand(cmd).Failed())
      {
        GetDocument()->GetObjectAccessor()->CancelTransaction();
        return;
      }

      const ezDocumentObject* pConeObject = GetDocument()->GetObjectAccessor()->GetObject(cmd.m_NewObjectGuid);

      // default speed
      {
        const ezHybridArray<ezDocumentObject*, 8>& children = pConeObject->GetChildren();

        for (auto pChild : children)
        {
          if (ezStringUtils::IsEqual(pChild->GetParentProperty(), "Speed"))
          {
            ezSetObjectPropertyCommand cmd2;
            cmd2.m_Object = pChild->GetGuid();
            cmd2.m_NewValue = 4.0f;
            cmd2.m_sProperty = "Value";
            cmd2.m_Index = 0;

            if (GetDocument()->GetCommandHistory()->AddCommand(cmd2).Failed())
            {
              GetDocument()->GetObjectAccessor()->CancelTransaction();
              return;
            }

            break;
          }
        }
      }
    }

    // add color initializer
    {
      ezAddObjectCommand cmd;
      cmd.m_Parent = systemGuid;
      cmd.m_Index = -1;
      cmd.m_pType = ezGetStaticRTTI<ezParticleInitializerFactory_RandomColor>();
      cmd.m_sParentProperty = "Initializers";

      if (GetDocument()->GetCommandHistory()->AddCommand(cmd).Failed())
      {
        GetDocument()->GetObjectAccessor()->CancelTransaction();
        return;
      }

      // color 1
      {
        ezSetObjectPropertyCommand cmd2;
        cmd2.m_Object = cmd.m_NewObjectGuid;
        cmd2.m_sProperty = "Color1";
        cmd2.m_NewValue = ezColor::Red;

        if (GetDocument()->GetCommandHistory()->AddCommand(cmd2).Failed())
        {
          GetDocument()->GetObjectAccessor()->CancelTransaction();
          return;
        }
      }

      // color 2
      {
        ezSetObjectPropertyCommand cmd2;
        cmd2.m_Object = cmd.m_NewObjectGuid;
        cmd2.m_sProperty = "Color2";
        cmd2.m_NewValue = ezColor::Yellow;

        if (GetDocument()->GetCommandHistory()->AddCommand(cmd2).Failed())
        {
          GetDocument()->GetObjectAccessor()->CancelTransaction();
          return;
        }
      }
    }

    // add gravity behavior
    {
      ezAddObjectCommand cmd;
      cmd.m_Parent = systemGuid;
      cmd.m_Index = -1;
      cmd.m_pType = ezGetStaticRTTI<ezParticleBehaviorFactory_Gravity>();
      cmd.m_sParentProperty = "Behaviors";

      if (GetDocument()->GetCommandHistory()->AddCommand(cmd).Failed())
      {
        GetDocument()->GetObjectAccessor()->CancelTransaction();
        return;
      }
    }

    // add quad renderer
    {
      ezAddObjectCommand cmd;
      cmd.m_Parent = systemGuid;
      cmd.m_Index = -1;
      cmd.m_pType = ezGetStaticRTTI<ezParticleTypeQuadFactory>();
      cmd.m_sParentProperty = "Types";

      if (GetDocument()->GetCommandHistory()->AddCommand(cmd).Failed())
      {
        GetDocument()->GetObjectAccessor()->CancelTransaction();
        return;
      }
    }
  }

  GetDocument()->GetObjectAccessor()->FinishTransaction();
}

void ezQtParticleEffectAssetDocumentWindow::onRemoveSystem(bool)
{
  const int index = m_pSystemsCombo->findText(m_sSelectedSystem.GetData());
  if (index < 0)
    return;

  const ezDocumentObject* pObject = static_cast<ezDocumentObject*>(m_pSystemsCombo->itemData(index).value<void*>());

  ezDocumentObject* pRootObject = GetParticleDocument()->GetObjectManager()->GetRootObject()->GetChildren()[0];

  GetDocument()->GetObjectAccessor()->StartTransaction("Rename Particle System");

  ezRemoveObjectCommand cmd;
  cmd.m_Object = pObject->GetGuid();

  if (GetDocument()->GetCommandHistory()->AddCommand(cmd).Failed())
  {
    GetDocument()->GetObjectAccessor()->CancelTransaction();
    return;
  }

  GetDocument()->GetObjectAccessor()->FinishTransaction();
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

  ezDocumentObject* pRootObject = GetParticleDocument()->GetObjectManager()->GetRootObject()->GetChildren()[0];

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
}

void ezQtParticleEffectAssetDocumentWindow::UpdatePreview()
{
  if (ezEditorEngineProcessConnection::GetSingleton()->IsProcessCrashed())
    return;

  ezEditorEngineResourceUpdateMsg msg;
  msg.m_sResourceType = "Particle";

  ezMemoryStreamStorage streamStorage;
  ezMemoryStreamWriter memoryWriter(&streamStorage);

  // Write Path
  ezStringBuilder sAbsFilePath = GetParticleDocument()->GetDocumentPath();
  sAbsFilePath.ChangeFileExtension("ezParticleEffect");
  // Write Header
  memoryWriter << sAbsFilePath;
  const ezUInt64 uiHash = ezAssetCurator::GetSingleton()->GetAssetDependencyHash(GetParticleDocument()->GetGuid());
  ezAssetFileHeader AssetHeader;
  AssetHeader.SetFileHashAndVersion(uiHash, GetParticleDocument()->GetAssetTypeVersion());
  AssetHeader.Write(memoryWriter);
  // Write Asset Data
  GetParticleDocument()->WriteParticleEffectAsset(memoryWriter, ezAssetCurator::GetSingleton()->GetActivePlatform());
  msg.m_Data = ezArrayPtr<const ezUInt8>(streamStorage.GetData(), streamStorage.GetStorageSize());

  GetEditorEngineConnection()->SendMessage(&msg);
}

void ezQtParticleEffectAssetDocumentWindow::PropertyEventHandler(const ezDocumentObjectPropertyEvent& e)
{
  if (e.m_sProperty == "Name" || e.m_sProperty == "ParticleSystems")
  {
    UpdateSystemList();
  }

  UpdatePreview();
}

void ezQtParticleEffectAssetDocumentWindow::StructureEventHandler(const ezDocumentObjectStructureEvent& e)
{
  switch (e.m_EventType)
  {
    case ezDocumentObjectStructureEvent::Type::AfterObjectAdded:
    case ezDocumentObjectStructureEvent::Type::AfterObjectMoved2:
    case ezDocumentObjectStructureEvent::Type::AfterObjectRemoved:
      UpdateSystemList();
      UpdatePreview();
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
  m_ParticleSystems.Clear();

  ezDocumentObject* pRootObject = GetParticleDocument()->GetObjectManager()->GetRootObject()->GetChildren()[0];

  ezStringBuilder s;

  for (ezDocumentObject* pChild : pRootObject->GetChildren())
  {
    if (ezStringUtils::IsEqual(pChild->GetParentProperty(), "ParticleSystems"))
    {
      s = pChild->GetTypeAccessor().GetValue("Name").ConvertTo<ezString>();
      m_ParticleSystems[s] = pChild;
    }
  }

  {
    ezQtScopedBlockSignals _1(m_pSystemsCombo);
    m_pSystemsCombo->clear();

    for (auto it = m_ParticleSystems.GetIterator(); it.IsValid(); ++it)
    {
      m_pSystemsCombo->addItem(it.Key().GetData(), qVariantFromValue<void*>(it.Value()));
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
  ezEditorEngineRestoreResourceMsg msg;
  GetEditorEngineConnection()->SendMessage(&msg);
}
