#include <PCH.h>
#include <EditorPluginAssets/PropertyAnimAsset/PropertyAnimAssetWindow.moc.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <GuiFoundation/Widgets/ImageWidget.moc.h>
#include <GuiFoundation/DockPanels/DocumentPanel.moc.h>
#include <EditorPluginAssets/PropertyAnimAsset/PropertyAnimModel.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <QLabel>
#include <QLayout>
#include <Foundation/Image/ImageConversion.h>
#include <QTreeView>
#include <GuiFoundation/Widgets/Curve1DEditorWidget.moc.h>

ezQtPropertyAnimAssetDocumentWindow::ezQtPropertyAnimAssetDocumentWindow(ezDocument* pDocument) : ezQtDocumentWindow(pDocument)
{
  // Menu Bar
  {
    ezQtMenuBarActionMapView* pMenuBar = static_cast<ezQtMenuBarActionMapView*>(menuBar());
    ezActionContext context;
    context.m_sMapping = "PropertyAnimAssetMenuBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pMenuBar->SetActionContext(context);
  }

  // Tool Bar
  {
    ezQtToolBarActionMapView* pToolBar = new ezQtToolBarActionMapView("Toolbar", this);
    ezActionContext context;
    context.m_sMapping = "PropertyAnimAssetToolBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pToolBar->SetActionContext(context);
    pToolBar->setObjectName("PropertyAnimAssetWindowToolBar");
    addToolBar(pToolBar);
  }

  // Property Grid
  {
    ezQtDocumentPanel* pPanel = new ezQtDocumentPanel(this);
    pPanel->setObjectName("PropertyAnimAssetDockWidget");
    pPanel->setWindowTitle("Properties");
    pPanel->show();

    ezQtPropertyGridWidget* pPropertyGrid = new ezQtPropertyGridWidget(pPanel, pDocument);
    pPanel->setWidget(pPropertyGrid);

    addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, pPanel);
  }

  // Property Tree View
  {
    ezQtDocumentPanel* pPanel = new ezQtDocumentPanel(this);
    pPanel->setObjectName("PropertyAnimPropertiesDockWidget");
    pPanel->setWindowTitle("Properties");
    pPanel->show();

    m_pPropertyTreeView = new QTreeView(pPanel);
    pPanel->setWidget(m_pPropertyTreeView);

    addDockWidget(Qt::DockWidgetArea::LeftDockWidgetArea, pPanel);
  }

  // Property Model
  {
    m_pPropertiesModel = new ezQtPropertyAnimModel(static_cast<ezPropertyAnimAssetDocument*>(pDocument), this);
    m_pPropertyTreeView->setModel(m_pPropertiesModel);
  }

  // Float Curve Panel
  {
    ezQtDocumentPanel* pPanel = new ezQtDocumentPanel(this);
    pPanel->setObjectName("PropertyAnimFloatCurveDockWidget");
    pPanel->setWindowTitle("Curves");
    pPanel->show();

    ezQtCurve1DEditorWidget* pCurveEditor = new ezQtCurve1DEditorWidget(pPanel);
    pPanel->setWidget(pCurveEditor);

    addDockWidget(Qt::DockWidgetArea::BottomDockWidgetArea, pPanel);
  }

  pDocument->GetSelectionManager()->SetSelection(pDocument->GetObjectManager()->GetRootObject()->GetChildren()[0]);

  FinishWindowCreation();
}

ezQtPropertyAnimAssetDocumentWindow::~ezQtPropertyAnimAssetDocumentWindow()
{
}
