#include <PCH.h>
#include <EditorPluginAssets/ColorGradientAsset/ColorGradientAssetWindow.moc.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <GuiFoundation/Widgets/ImageWidget.moc.h>
#include <GuiFoundation/DockPanels/DocumentPanel.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <QLabel>
#include <QLayout>
#include <Foundation/Image/ImageConversion.h>
#include <GuiFoundation/Widgets/ColorGradientEditorWidget.moc.h>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <EditorPluginAssets/ColorGradientAsset/ColorGradientAsset.h>

ezQtColorGradientAssetDocumentWindow::ezQtColorGradientAssetDocumentWindow(ezDocument* pDocument) : ezQtDocumentWindow(pDocument)
{
  GetDocument()->GetObjectManager()->m_PropertyEvents.AddEventHandler(ezMakeDelegate(&ezQtColorGradientAssetDocumentWindow::PropertyEventHandler, this));
  GetDocument()->GetObjectManager()->m_StructureEvents.AddEventHandler(ezMakeDelegate(&ezQtColorGradientAssetDocumentWindow::StructureEventHandler, this));

  // Menu Bar
  {
    ezQtMenuBarActionMapView* pMenuBar = static_cast<ezQtMenuBarActionMapView*>(menuBar());
    ezActionContext context;
    context.m_sMapping = "ColorGradientAssetMenuBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pMenuBar->SetActionContext(context);
  }

  // Tool Bar
  {
    ezQtToolBarActionMapView* pToolBar = new ezQtToolBarActionMapView("Toolbar", this);
    ezActionContext context;
    context.m_sMapping = "ColorGradientAssetToolBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pToolBar->SetActionContext(context);
    pToolBar->setObjectName("ColorGradientAssetWindowToolBar");
    addToolBar(pToolBar);
  }

  m_bShowFirstTime = true;
  m_pGradientEditor = new ezQtColorGradientEditorWidget(this);

  QWidget* pContainer = new QWidget(this);
  pContainer->setLayout(new QVBoxLayout(this));
  pContainer->layout()->addItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding));
  pContainer->layout()->addWidget(m_pGradientEditor);
  pContainer->layout()->addItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding));

  setCentralWidget(pContainer);

  connect(m_pGradientEditor, &ezQtColorGradientEditorWidget::ColorCpAdded, this, &ezQtColorGradientAssetDocumentWindow::onGradientColorCpAdded);
  connect(m_pGradientEditor, &ezQtColorGradientEditorWidget::ColorCpMoved, this, &ezQtColorGradientAssetDocumentWindow::onGradientColorCpMoved);
  connect(m_pGradientEditor, &ezQtColorGradientEditorWidget::ColorCpDeleted, this, &ezQtColorGradientAssetDocumentWindow::onGradientColorCpDeleted);
  connect(m_pGradientEditor, &ezQtColorGradientEditorWidget::ColorCpChanged, this, &ezQtColorGradientAssetDocumentWindow::onGradientColorCpChanged);

  connect(m_pGradientEditor, &ezQtColorGradientEditorWidget::AlphaCpAdded, this, &ezQtColorGradientAssetDocumentWindow::onGradientAlphaCpAdded);
  connect(m_pGradientEditor, &ezQtColorGradientEditorWidget::AlphaCpMoved, this, &ezQtColorGradientAssetDocumentWindow::onGradientAlphaCpMoved);
  connect(m_pGradientEditor, &ezQtColorGradientEditorWidget::AlphaCpDeleted, this, &ezQtColorGradientAssetDocumentWindow::onGradientAlphaCpDeleted);
  connect(m_pGradientEditor, &ezQtColorGradientEditorWidget::AlphaCpChanged, this, &ezQtColorGradientAssetDocumentWindow::onGradientAlphaCpChanged);

  connect(m_pGradientEditor, &ezQtColorGradientEditorWidget::IntensityCpAdded, this, &ezQtColorGradientAssetDocumentWindow::onGradientIntensityCpAdded);
  connect(m_pGradientEditor, &ezQtColorGradientEditorWidget::IntensityCpMoved, this, &ezQtColorGradientAssetDocumentWindow::onGradientIntensityCpMoved);
  connect(m_pGradientEditor, &ezQtColorGradientEditorWidget::IntensityCpDeleted, this, &ezQtColorGradientAssetDocumentWindow::onGradientIntensityCpDeleted);
  connect(m_pGradientEditor, &ezQtColorGradientEditorWidget::IntensityCpChanged, this, &ezQtColorGradientAssetDocumentWindow::onGradientIntensityCpChanged);

  connect(m_pGradientEditor, &ezQtColorGradientEditorWidget::BeginOperation, this, &ezQtColorGradientAssetDocumentWindow::onGradientBeginOperation);
  connect(m_pGradientEditor, &ezQtColorGradientEditorWidget::EndOperation, this, &ezQtColorGradientAssetDocumentWindow::onGradientEndOperation);

  connect(m_pGradientEditor, &ezQtColorGradientEditorWidget::NormalizeRange, this, &ezQtColorGradientAssetDocumentWindow::onGradientNormalizeRange);

  // property grid, if needed
  if (false)
  {
    ezQtDocumentPanel* pPropertyPanel = new ezQtDocumentPanel(this);
    pPropertyPanel->setObjectName("ColorGradientAssetDockWidget");
    pPropertyPanel->setWindowTitle("ColorGradient Properties");
    pPropertyPanel->show();

    ezQtPropertyGridWidget* pPropertyGrid = new ezQtPropertyGridWidget(pPropertyPanel, pDocument);
    pPropertyPanel->setWidget(pPropertyGrid);

    addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, pPropertyPanel);

    pDocument->GetSelectionManager()->SetSelection(pDocument->GetObjectManager()->GetRootObject()->GetChildren()[0]);
  }

  FinishWindowCreation();

  UpdatePreview();
}

ezQtColorGradientAssetDocumentWindow::~ezQtColorGradientAssetDocumentWindow()
{
  GetDocument()->GetObjectManager()->m_PropertyEvents.RemoveEventHandler(ezMakeDelegate(&ezQtColorGradientAssetDocumentWindow::PropertyEventHandler, this));
  GetDocument()->GetObjectManager()->m_StructureEvents.RemoveEventHandler(ezMakeDelegate(&ezQtColorGradientAssetDocumentWindow::StructureEventHandler, this));
}


void ezQtColorGradientAssetDocumentWindow::onGradientColorCpAdded(double posX, const ezColorGammaUB& color)
{
  ezColorGradientAssetDocument* pDoc = static_cast<ezColorGradientAssetDocument*>(GetDocument());

  ezCommandHistory* history = GetDocument()->GetCommandHistory();
  history->StartTransaction("Add Color Control Point");

  ezAddObjectCommand cmdAdd;
  cmdAdd.m_Parent = pDoc->GetPropertyObject()->GetGuid();
  cmdAdd.m_NewObjectGuid.CreateNewUuid();
  cmdAdd.m_sParentProperty = "ColorCPs";
  cmdAdd.m_pType = ezGetStaticRTTI<ezColorControlPoint>();
  cmdAdd.m_Index = -1;

  history->AddCommand(cmdAdd);

  ezSetObjectPropertyCommand cmdSet;
  cmdSet.m_Object = cmdAdd.m_NewObjectGuid;

  cmdSet.m_sProperty = "Tick";
  cmdSet.m_NewValue = pDoc->GetProperties()->TickFromTime(posX);
  history->AddCommand(cmdSet);

  cmdSet.m_sProperty = "Red";
  cmdSet.m_NewValue = color.r;
  history->AddCommand(cmdSet);

  cmdSet.m_sProperty = "Green";
  cmdSet.m_NewValue = color.g;
  history->AddCommand(cmdSet);

  cmdSet.m_sProperty = "Blue";
  cmdSet.m_NewValue = color.b;
  history->AddCommand(cmdSet);

  history->FinishTransaction();
}


void ezQtColorGradientAssetDocumentWindow::onGradientAlphaCpAdded(double posX, ezUInt8 alpha)
{
  ezColorGradientAssetDocument* pDoc = static_cast<ezColorGradientAssetDocument*>(GetDocument());

  ezCommandHistory* history = GetDocument()->GetCommandHistory();
  history->StartTransaction("Add Alpha Control Point");

  ezAddObjectCommand cmdAdd;
  cmdAdd.m_Parent = pDoc->GetPropertyObject()->GetGuid();
  cmdAdd.m_NewObjectGuid.CreateNewUuid();
  cmdAdd.m_sParentProperty = "AlphaCPs";
  cmdAdd.m_pType = ezGetStaticRTTI<ezAlphaControlPoint>();
  cmdAdd.m_Index = -1;

  history->AddCommand(cmdAdd);

  ezSetObjectPropertyCommand cmdSet;
  cmdSet.m_Object = cmdAdd.m_NewObjectGuid;

  cmdSet.m_sProperty = "Tick";
  cmdSet.m_NewValue = pDoc->GetProperties()->TickFromTime(posX);
  history->AddCommand(cmdSet);

  cmdSet.m_sProperty = "Alpha";
  cmdSet.m_NewValue = alpha;
  history->AddCommand(cmdSet);

  history->FinishTransaction();
}


void ezQtColorGradientAssetDocumentWindow::onGradientIntensityCpAdded(double posX, float intensity)
{
  ezColorGradientAssetDocument* pDoc = static_cast<ezColorGradientAssetDocument*>(GetDocument());

  ezCommandHistory* history = GetDocument()->GetCommandHistory();
  history->StartTransaction("Add Intensity Control Point");

  ezAddObjectCommand cmdAdd;
  cmdAdd.m_Parent = pDoc->GetPropertyObject()->GetGuid();
  cmdAdd.m_NewObjectGuid.CreateNewUuid();
  cmdAdd.m_sParentProperty = "IntensityCPs";
  cmdAdd.m_pType = ezGetStaticRTTI<ezIntensityControlPoint>();
  cmdAdd.m_Index = -1;

  history->AddCommand(cmdAdd);

  ezSetObjectPropertyCommand cmdSet;
  cmdSet.m_Object = cmdAdd.m_NewObjectGuid;

  cmdSet.m_sProperty = "Tick";
  cmdSet.m_NewValue = pDoc->GetProperties()->TickFromTime(posX);
  history->AddCommand(cmdSet);

  cmdSet.m_sProperty = "Intensity";
  cmdSet.m_NewValue = intensity;
  history->AddCommand(cmdSet);

  history->FinishTransaction();
}

void ezQtColorGradientAssetDocumentWindow::MoveCP(ezInt32 idx, double newPosX, const char* szArrayName)
{
  ezColorGradientAssetDocument* pDoc = static_cast<ezColorGradientAssetDocument*>(GetDocument());

  auto pProp = pDoc->GetPropertyObject();

  ezVariant objGuid = pProp->GetTypeAccessor().GetValue(szArrayName, idx);

  ezCommandHistory* history = GetDocument()->GetCommandHistory();
  history->StartTransaction("Move Control Point");

  ezSetObjectPropertyCommand cmdSet;
  cmdSet.m_Object = objGuid.Get<ezUuid>();

  cmdSet.m_sProperty = "Tick";
  cmdSet.m_NewValue = pDoc->GetProperties()->TickFromTime(newPosX);
  history->AddCommand(cmdSet);

  history->FinishTransaction();
}

void ezQtColorGradientAssetDocumentWindow::onGradientColorCpMoved(ezInt32 idx, double newPosX)
{
  MoveCP(idx, newPosX, "ColorCPs");
}

void ezQtColorGradientAssetDocumentWindow::onGradientAlphaCpMoved(ezInt32 idx, double newPosX)
{
  MoveCP(idx, newPosX, "AlphaCPs");
}


void ezQtColorGradientAssetDocumentWindow::onGradientIntensityCpMoved(ezInt32 idx, double newPosX)
{
  MoveCP(idx, newPosX, "IntensityCPs");
}

void ezQtColorGradientAssetDocumentWindow::RemoveCP(ezInt32 idx, const char* szArrayName)
{
  ezColorGradientAssetDocument* pDoc = static_cast<ezColorGradientAssetDocument*>(GetDocument());

  auto pProp = pDoc->GetPropertyObject();

  ezVariant objGuid = pProp->GetTypeAccessor().GetValue(szArrayName, idx);

  ezCommandHistory* history = GetDocument()->GetCommandHistory();
  history->StartTransaction("Remove Control Point");

  ezRemoveObjectCommand cmdSet;
  cmdSet.m_Object = objGuid.Get<ezUuid>();
  history->AddCommand(cmdSet);

  history->FinishTransaction();
}

void ezQtColorGradientAssetDocumentWindow::onGradientColorCpDeleted(ezInt32 idx)
{
  RemoveCP(idx, "ColorCPs");
}


void ezQtColorGradientAssetDocumentWindow::onGradientAlphaCpDeleted(ezInt32 idx)
{
  RemoveCP(idx, "AlphaCPs");
}


void ezQtColorGradientAssetDocumentWindow::onGradientIntensityCpDeleted(ezInt32 idx)
{
  RemoveCP(idx, "IntensityCPs");
}


void ezQtColorGradientAssetDocumentWindow::onGradientColorCpChanged(ezInt32 idx, const ezColorGammaUB& color)
{
  ezColorGradientAssetDocument* pDoc = static_cast<ezColorGradientAssetDocument*>(GetDocument());

  auto pProp = pDoc->GetPropertyObject();
  ezVariant objGuid = pProp->GetTypeAccessor().GetValue("ColorCPs", idx);

  ezCommandHistory* history = GetDocument()->GetCommandHistory();
  history->StartTransaction("Change Color");

  ezSetObjectPropertyCommand cmdSet;
  cmdSet.m_Object = objGuid.Get<ezUuid>();

  cmdSet.m_sProperty = "Red";
  cmdSet.m_NewValue = color.r;
  history->AddCommand(cmdSet);

  cmdSet.m_sProperty = "Green";
  cmdSet.m_NewValue = color.g;
  history->AddCommand(cmdSet);

  cmdSet.m_sProperty = "Blue";
  cmdSet.m_NewValue = color.b;
  history->AddCommand(cmdSet);

  history->FinishTransaction();
}


void ezQtColorGradientAssetDocumentWindow::onGradientAlphaCpChanged(ezInt32 idx, ezUInt8 alpha)
{
  ezColorGradientAssetDocument* pDoc = static_cast<ezColorGradientAssetDocument*>(GetDocument());

  auto pProp = pDoc->GetPropertyObject();
  ezVariant objGuid = pProp->GetTypeAccessor().GetValue("AlphaCPs", idx);

  ezCommandHistory* history = GetDocument()->GetCommandHistory();
  history->StartTransaction("Change Alpha");

  ezSetObjectPropertyCommand cmdSet;
  cmdSet.m_Object = objGuid.Get<ezUuid>();

  cmdSet.m_sProperty = "Alpha";
  cmdSet.m_NewValue = alpha;
  history->AddCommand(cmdSet);

  history->FinishTransaction();
}

void ezQtColorGradientAssetDocumentWindow::onGradientIntensityCpChanged(ezInt32 idx, float intensity)
{
  ezColorGradientAssetDocument* pDoc = static_cast<ezColorGradientAssetDocument*>(GetDocument());

  auto pProp = pDoc->GetPropertyObject();
  ezVariant objGuid = pProp->GetTypeAccessor().GetValue("IntensityCPs", idx);

  ezCommandHistory* history = GetDocument()->GetCommandHistory();
  history->StartTransaction("Change Intensity");

  ezSetObjectPropertyCommand cmdSet;
  cmdSet.m_Object = objGuid.Get<ezUuid>();

  cmdSet.m_sProperty = "Intensity";
  cmdSet.m_NewValue = intensity;
  history->AddCommand(cmdSet);

  history->FinishTransaction();
}


void ezQtColorGradientAssetDocumentWindow::onGradientBeginOperation()
{
  ezCommandHistory* history = GetDocument()->GetCommandHistory();
  history->BeginTemporaryCommands("Modify Gradient");
}


void ezQtColorGradientAssetDocumentWindow::onGradientEndOperation(bool commit)
{
  ezCommandHistory* history = GetDocument()->GetCommandHistory();

  if (commit)
    history->FinishTemporaryCommands();
  else
    history->CancelTemporaryCommands();
}


void ezQtColorGradientAssetDocumentWindow::onGradientNormalizeRange()
{
  if (ezQtUiServices::GetSingleton()->MessageBoxQuestion("This will adjust the positions of all control points, such that the minimum is at 0 and the maximum at 1.\n\nContinue?", QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No, QMessageBox::StandardButton::Yes) != QMessageBox::StandardButton::Yes)
    return;

  ezColorGradientAssetDocument* pDoc = static_cast<ezColorGradientAssetDocument*>(GetDocument());

  ezColorGradient GradientData;
  pDoc->GetProperties()->FillGradientData(GradientData);

  double minX, maxX;
  if (!GradientData.GetExtents(minX, maxX))
    return;

  if ((minX == 0 && maxX == 1) || (minX >= maxX))
    return;

  ezCommandHistory* history = GetDocument()->GetCommandHistory();

  const float rangeNorm = 1.0f / (maxX - minX);

  history->StartTransaction("Normalize Gradient Range");

  ezUInt32 numRgb, numAlpha, numInt;
  GradientData.GetNumControlPoints(numRgb, numAlpha, numInt);

  for (ezUInt32 i = 0; i < numRgb; ++i)
  {
    float x = GradientData.GetColorControlPoint(i).m_PosX;
    x -= minX;
    x *= rangeNorm;

    MoveCP(i, x, "ColorCPs");
  }

  for (ezUInt32 i = 0; i < numAlpha; ++i)
  {
    float x = GradientData.GetAlphaControlPoint(i).m_PosX;
    x -= minX;
    x *= rangeNorm;

    MoveCP(i, x, "AlphaCPs");
  }

  for (ezUInt32 i = 0; i < numInt; ++i)
  {
    float x = GradientData.GetIntensityControlPoint(i).m_PosX;
    x -= minX;
    x *= rangeNorm;

    MoveCP(i, x, "IntensityCPs");
  }

  history->FinishTransaction();

  m_pGradientEditor->FrameGradient();
}

void ezQtColorGradientAssetDocumentWindow::UpdatePreview()
{
  ezColorGradient GradientData;

  ezColorGradientAssetDocument* pDoc = static_cast<ezColorGradientAssetDocument*>(GetDocument());
  pDoc->GetProperties()->FillGradientData(GradientData);

  m_pGradientEditor->SetColorGradient(GradientData);

  if (m_bShowFirstTime)
  {
    m_bShowFirstTime = false;
    m_pGradientEditor->FrameGradient();
  }
}

void ezQtColorGradientAssetDocumentWindow::PropertyEventHandler(const ezDocumentObjectPropertyEvent& e)
{
  UpdatePreview();
}

void ezQtColorGradientAssetDocumentWindow::StructureEventHandler(const ezDocumentObjectStructureEvent& e)
{
  UpdatePreview();
}



