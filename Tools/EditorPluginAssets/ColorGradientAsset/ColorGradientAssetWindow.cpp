#include <PCH.h>
#include <EditorPluginAssets/ColorGradientAsset/ColorGradientAssetWindow.moc.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <GuiFoundation/Widgets/ImageWidget.moc.h>
#include <GuiFoundation/DockPanels/DocumentPanel.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <QLabel>
#include <QLayout>
#include <CoreUtils/Image/ImageConversion.h>
#include <GuiFoundation/Widgets/ColorGradientEditorWidget.moc.h>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <EditorPluginAssets/ColorGradientAsset/ColorGradientAsset.h>

ezColorGradientAssetDocumentWindow::ezColorGradientAssetDocumentWindow(ezDocument* pDocument) : ezQtDocumentWindow(pDocument)
{
  GetDocument()->GetObjectManager()->m_PropertyEvents.AddEventHandler(ezMakeDelegate(&ezColorGradientAssetDocumentWindow::PropertyEventHandler, this));
  GetDocument()->GetObjectManager()->m_StructureEvents.AddEventHandler(ezMakeDelegate(&ezColorGradientAssetDocumentWindow::StructureEventHandler, this));

  // Menu Bar
  {
    ezMenuBarActionMapView* pMenuBar = static_cast<ezMenuBarActionMapView*>(menuBar());
    ezActionContext context;
    context.m_sMapping = "ColorGradientAssetMenuBar";
    context.m_pDocument = pDocument;
    pMenuBar->SetActionContext(context);
  }

  // Tool Bar
  {
    ezToolBarActionMapView* pToolBar = new ezToolBarActionMapView("Toolbar", this);
    ezActionContext context;
    context.m_sMapping = "ColorGradientAssetToolBar";
    context.m_pDocument = pDocument;
    pToolBar->SetActionContext(context);
    pToolBar->setObjectName("ColorGradientAssetWindowToolBar");
    addToolBar(pToolBar);
  }

  m_pGradientEditor = new QColorGradientEditorWidget(this);

  QWidget* pContainer = new QWidget(this);
  pContainer->setLayout(new QVBoxLayout(this));
  pContainer->layout()->addItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding));
  pContainer->layout()->addWidget(m_pGradientEditor);
  pContainer->layout()->addItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding));

  setCentralWidget(pContainer);

  connect(m_pGradientEditor, &QColorGradientEditorWidget::ColorCpAdded, this, &ezColorGradientAssetDocumentWindow::onGradientColorCpAdded);
  connect(m_pGradientEditor, &QColorGradientEditorWidget::ColorCpMoved, this, &ezColorGradientAssetDocumentWindow::onGradientColorCpMoved);
  connect(m_pGradientEditor, &QColorGradientEditorWidget::ColorCpDeleted, this, &ezColorGradientAssetDocumentWindow::onGradientColorCpDeleted);
  connect(m_pGradientEditor, &QColorGradientEditorWidget::ColorCpChanged, this, &ezColorGradientAssetDocumentWindow::onGradientColorCpChanged);

  connect(m_pGradientEditor, &QColorGradientEditorWidget::AlphaCpAdded, this, &ezColorGradientAssetDocumentWindow::onGradientAlphaCpAdded);
  connect(m_pGradientEditor, &QColorGradientEditorWidget::AlphaCpMoved, this, &ezColorGradientAssetDocumentWindow::onGradientAlphaCpMoved);
  connect(m_pGradientEditor, &QColorGradientEditorWidget::AlphaCpDeleted, this, &ezColorGradientAssetDocumentWindow::onGradientAlphaCpDeleted);
  connect(m_pGradientEditor, &QColorGradientEditorWidget::AlphaCpChanged, this, &ezColorGradientAssetDocumentWindow::onGradientAlphaCpChanged);

  connect(m_pGradientEditor, &QColorGradientEditorWidget::IntensityCpAdded, this, &ezColorGradientAssetDocumentWindow::onGradientIntensityCpAdded);
  connect(m_pGradientEditor, &QColorGradientEditorWidget::IntensityCpMoved, this, &ezColorGradientAssetDocumentWindow::onGradientIntensityCpMoved);
  connect(m_pGradientEditor, &QColorGradientEditorWidget::IntensityCpDeleted, this, &ezColorGradientAssetDocumentWindow::onGradientIntensityCpDeleted);
  connect(m_pGradientEditor, &QColorGradientEditorWidget::IntensityCpChanged, this, &ezColorGradientAssetDocumentWindow::onGradientIntensityCpChanged);

  connect(m_pGradientEditor, &QColorGradientEditorWidget::BeginOperation, this, &ezColorGradientAssetDocumentWindow::onGradientBeginOperation);
  connect(m_pGradientEditor, &QColorGradientEditorWidget::EndOperation, this, &ezColorGradientAssetDocumentWindow::onGradientEndOperation);

  connect(m_pGradientEditor, &QColorGradientEditorWidget::NormalizeRange, this, &ezColorGradientAssetDocumentWindow::onGradientNormalizeRange);

  {
    ezDocumentPanel* pPropertyPanel = new ezDocumentPanel(this);
    pPropertyPanel->setObjectName("ColorGradientAssetDockWidget");
    pPropertyPanel->setWindowTitle("ColorGradient Properties");
    pPropertyPanel->show();

    ezPropertyGridWidget* pPropertyGrid = new ezPropertyGridWidget(pPropertyPanel, pDocument);
    pPropertyPanel->setWidget(pPropertyGrid);

    addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, pPropertyPanel);

    pDocument->GetSelectionManager()->SetSelection(pDocument->GetObjectManager()->GetRootObject()->GetChildren()[0]);
  }

  FinishWindowCreation();

  UpdatePreview();
}

ezColorGradientAssetDocumentWindow::~ezColorGradientAssetDocumentWindow()
{
  GetDocument()->GetObjectManager()->m_PropertyEvents.RemoveEventHandler(ezMakeDelegate(&ezColorGradientAssetDocumentWindow::PropertyEventHandler, this));
  GetDocument()->GetObjectManager()->m_StructureEvents.RemoveEventHandler(ezMakeDelegate(&ezColorGradientAssetDocumentWindow::StructureEventHandler, this));
}


void ezColorGradientAssetDocumentWindow::onGradientColorCpAdded(float posX, const ezColorGammaUB& color)
{
  ezColorGradientAssetDocument* pDoc = static_cast<ezColorGradientAssetDocument*>(GetDocument());

  ezCommandHistory* history = GetDocument()->GetCommandHistory();
  history->StartTransaction();

  ezAddObjectCommand cmdAdd;
  cmdAdd.m_Parent = pDoc->GetPropertyObject()->GetGuid();
  cmdAdd.m_NewObjectGuid.CreateNewUuid();
  cmdAdd.m_sParentProperty = "Color CPs";
  cmdAdd.m_pType = ezGetStaticRTTI<ezColorControlPoint>();
  cmdAdd.m_Index = -1;

  history->AddCommand(cmdAdd);

  ezSetObjectPropertyCommand cmdSet;
  cmdSet.m_Object = cmdAdd.m_NewObjectGuid;

  cmdSet.m_sPropertyPath = "Position";
  cmdSet.m_NewValue = posX;
  history->AddCommand(cmdSet);

  cmdSet.m_sPropertyPath = "Red";
  cmdSet.m_NewValue = color.r;
  history->AddCommand(cmdSet);

  cmdSet.m_sPropertyPath = "Green";
  cmdSet.m_NewValue = color.g;
  history->AddCommand(cmdSet);

  cmdSet.m_sPropertyPath = "Blue";
  cmdSet.m_NewValue = color.b;
  history->AddCommand(cmdSet);

  history->FinishTransaction();
}


void ezColorGradientAssetDocumentWindow::onGradientAlphaCpAdded(float posX, ezUInt8 alpha)
{
  ezColorGradientAssetDocument* pDoc = static_cast<ezColorGradientAssetDocument*>(GetDocument());

  ezCommandHistory* history = GetDocument()->GetCommandHistory();
  history->StartTransaction();

  ezAddObjectCommand cmdAdd;
  cmdAdd.m_Parent = pDoc->GetPropertyObject()->GetGuid();
  cmdAdd.m_NewObjectGuid.CreateNewUuid();
  cmdAdd.m_sParentProperty = "Alpha CPs";
  cmdAdd.m_pType = ezGetStaticRTTI<ezAlphaControlPoint>();
  cmdAdd.m_Index = -1;

  history->AddCommand(cmdAdd);

  ezSetObjectPropertyCommand cmdSet;
  cmdSet.m_Object = cmdAdd.m_NewObjectGuid;

  cmdSet.m_sPropertyPath = "Position";
  cmdSet.m_NewValue = posX;
  history->AddCommand(cmdSet);

  cmdSet.m_sPropertyPath = "Alpha";
  cmdSet.m_NewValue = alpha;
  history->AddCommand(cmdSet);

  history->FinishTransaction();
}


void ezColorGradientAssetDocumentWindow::onGradientIntensityCpAdded(float posX, float intensity)
{
  ezColorGradientAssetDocument* pDoc = static_cast<ezColorGradientAssetDocument*>(GetDocument());

  ezCommandHistory* history = GetDocument()->GetCommandHistory();
  history->StartTransaction();

  ezAddObjectCommand cmdAdd;
  cmdAdd.m_Parent = pDoc->GetPropertyObject()->GetGuid();
  cmdAdd.m_NewObjectGuid.CreateNewUuid();
  cmdAdd.m_sParentProperty = "Intensity CPs";
  cmdAdd.m_pType = ezGetStaticRTTI<ezIntensityControlPoint>();
  cmdAdd.m_Index = -1;

  history->AddCommand(cmdAdd);

  ezSetObjectPropertyCommand cmdSet;
  cmdSet.m_Object = cmdAdd.m_NewObjectGuid;

  cmdSet.m_sPropertyPath = "Position";
  cmdSet.m_NewValue = posX;
  history->AddCommand(cmdSet);

  cmdSet.m_sPropertyPath = "Intensity";
  cmdSet.m_NewValue = intensity;
  history->AddCommand(cmdSet);

  history->FinishTransaction();
}

void ezColorGradientAssetDocumentWindow::MoveCP(ezInt32 idx, float newPosX, const char* szArrayName)
{
  ezColorGradientAssetDocument* pDoc = static_cast<ezColorGradientAssetDocument*>(GetDocument());

  auto pProp = pDoc->GetPropertyObject();

  ezVariant objGuid = pProp->GetTypeAccessor().GetValue(szArrayName, idx);

  ezCommandHistory* history = GetDocument()->GetCommandHistory();
  history->StartTransaction();

  ezSetObjectPropertyCommand cmdSet;
  cmdSet.m_Object = objGuid.Get<ezUuid>();

  cmdSet.m_sPropertyPath = "Position";
  cmdSet.m_NewValue = newPosX;
  history->AddCommand(cmdSet);

  history->FinishTransaction();
}

void ezColorGradientAssetDocumentWindow::onGradientColorCpMoved(ezInt32 idx, float newPosX)
{
  MoveCP(idx, newPosX, "Color CPs");
}

void ezColorGradientAssetDocumentWindow::onGradientAlphaCpMoved(ezInt32 idx, float newPosX)
{
  MoveCP(idx, newPosX, "Alpha CPs");
}


void ezColorGradientAssetDocumentWindow::onGradientIntensityCpMoved(ezInt32 idx, float newPosX)
{
  MoveCP(idx, newPosX, "Intensity CPs");
}

void ezColorGradientAssetDocumentWindow::RemoveCP(ezInt32 idx, const char* szArrayName)
{
  ezColorGradientAssetDocument* pDoc = static_cast<ezColorGradientAssetDocument*>(GetDocument());

  auto pProp = pDoc->GetPropertyObject();

  ezVariant objGuid = pProp->GetTypeAccessor().GetValue(szArrayName, idx);

  ezCommandHistory* history = GetDocument()->GetCommandHistory();
  history->StartTransaction();

  ezRemoveObjectCommand cmdSet;
  cmdSet.m_Object = objGuid.Get<ezUuid>();
  history->AddCommand(cmdSet);

  history->FinishTransaction();
}

void ezColorGradientAssetDocumentWindow::onGradientColorCpDeleted(ezInt32 idx)
{
  RemoveCP(idx, "Color CPs");
}


void ezColorGradientAssetDocumentWindow::onGradientAlphaCpDeleted(ezInt32 idx)
{
  RemoveCP(idx, "Alpha CPs");
}


void ezColorGradientAssetDocumentWindow::onGradientIntensityCpDeleted(ezInt32 idx)
{
  RemoveCP(idx, "Intensity CPs");
}


void ezColorGradientAssetDocumentWindow::onGradientColorCpChanged(ezInt32 idx, const ezColorGammaUB& color)
{
  ezColorGradientAssetDocument* pDoc = static_cast<ezColorGradientAssetDocument*>(GetDocument());

  auto pProp = pDoc->GetPropertyObject();
  ezVariant objGuid = pProp->GetTypeAccessor().GetValue("Color CPs", idx);

  ezCommandHistory* history = GetDocument()->GetCommandHistory();
  history->StartTransaction();

  ezSetObjectPropertyCommand cmdSet;
  cmdSet.m_Object = objGuid.Get<ezUuid>();

  cmdSet.m_sPropertyPath = "Red";
  cmdSet.m_NewValue = color.r;
  history->AddCommand(cmdSet);

  cmdSet.m_sPropertyPath = "Green";
  cmdSet.m_NewValue = color.g;
  history->AddCommand(cmdSet);

  cmdSet.m_sPropertyPath = "Blue";
  cmdSet.m_NewValue = color.b;
  history->AddCommand(cmdSet);

  history->FinishTransaction();
}


void ezColorGradientAssetDocumentWindow::onGradientAlphaCpChanged(ezInt32 idx, ezUInt8 alpha)
{
  ezColorGradientAssetDocument* pDoc = static_cast<ezColorGradientAssetDocument*>(GetDocument());

  auto pProp = pDoc->GetPropertyObject();
  ezVariant objGuid = pProp->GetTypeAccessor().GetValue("Alpha CPs", idx);

  ezCommandHistory* history = GetDocument()->GetCommandHistory();
  history->StartTransaction();

  ezSetObjectPropertyCommand cmdSet;
  cmdSet.m_Object = objGuid.Get<ezUuid>();

  cmdSet.m_sPropertyPath = "Alpha";
  cmdSet.m_NewValue = alpha;
  history->AddCommand(cmdSet);

  history->FinishTransaction();
}

void ezColorGradientAssetDocumentWindow::onGradientIntensityCpChanged(ezInt32 idx, float intensity)
{
  ezColorGradientAssetDocument* pDoc = static_cast<ezColorGradientAssetDocument*>(GetDocument());

  auto pProp = pDoc->GetPropertyObject();
  ezVariant objGuid = pProp->GetTypeAccessor().GetValue("Intensity CPs", idx);

  ezCommandHistory* history = GetDocument()->GetCommandHistory();
  history->StartTransaction();

  ezSetObjectPropertyCommand cmdSet;
  cmdSet.m_Object = objGuid.Get<ezUuid>();

  cmdSet.m_sPropertyPath = "Intensity";
  cmdSet.m_NewValue = intensity;
  history->AddCommand(cmdSet);

  history->FinishTransaction();
}


void ezColorGradientAssetDocumentWindow::onGradientBeginOperation()
{
  ezCommandHistory* history = GetDocument()->GetCommandHistory();
  history->BeginTemporaryCommands();
}


void ezColorGradientAssetDocumentWindow::onGradientEndOperation(bool commit)
{
  ezCommandHistory* history = GetDocument()->GetCommandHistory();

  if (commit)
    history->FinishTemporaryCommands();
  else
    history->CancelTemporaryCommands();
}


void ezColorGradientAssetDocumentWindow::onGradientNormalizeRange()
{
  ezColorGradientAssetDocument* pDoc = static_cast<ezColorGradientAssetDocument*>(GetDocument());

  ezColorGradient GradientData;
  pDoc->FillGradientData(GradientData);

  float minX, maxX;
  if (!GradientData.GetExtents(minX, maxX))
    return;

  if (minX == 0 && maxX == 1)
    return;

  ezCommandHistory* history = GetDocument()->GetCommandHistory();

  const float rangeNorm = 1.0f / (maxX - minX);

  history->StartTransaction();

  ezUInt32 numRgb, numAlpha, numInt;
  GradientData.GetNumControlPoints(numRgb, numAlpha, numInt);

  for (ezUInt32 i = 0; i < numRgb; ++i)
  {
    float x = GradientData.GetColorControlPoint(i).m_PosX;
    x -= minX;
    x *= rangeNorm;

    MoveCP(i, x, "Color CPs");
  }

  for (ezUInt32 i = 0; i < numAlpha; ++i)
  {
    float x = GradientData.GetAlphaControlPoint(i).m_PosX;
    x -= minX;
    x *= rangeNorm;

    MoveCP(i, x, "Alpha CPs");
  }

  for (ezUInt32 i = 0; i < numInt; ++i)
  {
    float x = GradientData.GetIntensityControlPoint(i).m_PosX;
    x -= minX;
    x *= rangeNorm;

    MoveCP(i, x, "Intensity CPs");
  }

  history->FinishTransaction();

  m_pGradientEditor->FrameGradient();
}

void ezColorGradientAssetDocumentWindow::UpdatePreview()
{
  ezColorGradient GradientData;

  ezColorGradientAssetDocument* pDoc = static_cast<ezColorGradientAssetDocument*>(GetDocument());
  pDoc->FillGradientData(GradientData);

  m_pGradientEditor->SetColorGradient(GradientData);
}

void ezColorGradientAssetDocumentWindow::PropertyEventHandler(const ezDocumentObjectPropertyEvent& e)
{
  UpdatePreview();
}

void ezColorGradientAssetDocumentWindow::StructureEventHandler(const ezDocumentObjectStructureEvent& e)
{
  UpdatePreview();
}



