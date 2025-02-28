#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorPluginScene/Panels/ScenegraphPanel/ScenegraphModel.moc.h>
#include <EditorPluginScene/Panels/ScenegraphPanel/ScenegraphPanel.moc.h>
#include <EditorPluginScene/Scene/Scene2Document.h>
#include <GuiFoundation/Widgets/SearchWidget.moc.h>
#include <QLayout>
#include <QStackedWidget>

namespace
{
  std::unique_ptr<ezQtDocumentTreeModel> CreateGameObjectTreeModel(ezSceneDocument* pDocument)
  {
    std::unique_ptr<ezQtDocumentTreeModel> pModel(new ezQtScenegraphModel(pDocument->GetObjectManager()));
    pModel->AddAdapter(new ezQtDummyAdapter(pDocument->GetObjectManager(), ezGetStaticRTTI<ezDocumentRoot>(), "Children"));
    pModel->AddAdapter(new ezQtGameObjectAdapter(pDocument->GetObjectManager()));
    return std::move(pModel);
  }

  std::unique_ptr<ezQtDocumentTreeModel> CreateSceneTreeModel(ezScene2Document* pDocument)
  {
    std::unique_ptr<ezQtDocumentTreeModel> pModel(new ezQtScenegraphModel(pDocument->GetSceneObjectManager()));
    pModel->AddAdapter(new ezQtDummyAdapter(pDocument->GetSceneObjectManager(), ezGetStaticRTTI<ezDocumentRoot>(), "Children"));
    pModel->AddAdapter(new ezQtGameObjectAdapter(pDocument->GetSceneObjectManager(), pDocument->GetSceneDocumentObjectMetaData(), pDocument->GetSceneGameObjectMetaData()));
    return std::move(pModel);
  }
} // namespace

ezQtScenegraphPanel::ezQtScenegraphPanel(ads::CDockManager* pDockManager, QWidget* pParent, ezSceneDocument* pDocument)
  : ezQtDocumentPanel(pDockManager, pParent, pDocument)
{
  setObjectName("ezQtScenegraphPanel");
  setWindowTitle("Scenegraph");
  m_pSceneDocument = pDocument;

  m_pStack = new QStackedWidget(this);
  m_pStack->setObjectName("QStackedWidget");
  m_pStack->setContentsMargins(0, 0, 0, 0);
  m_pStack->layout()->setContentsMargins(0, 0, 0, 0);
  setWidget(m_pStack);

  auto pCustomModel = CreateGameObjectTreeModel(pDocument);
  m_pMainGameObjectWidget = new ezQtGameObjectWidget(this, pDocument, "EditorPluginScene_ScenegraphContextMenu", std::move(pCustomModel));
  m_pStack->addWidget(m_pMainGameObjectWidget);
}

ezQtScenegraphPanel::ezQtScenegraphPanel(ads::CDockManager* pDockManager, QWidget* pParent, ezScene2Document* pDocument)
  : ezQtDocumentPanel(pDockManager, pParent, pDocument)
{
  setObjectName("ezQtScenegraphPanel");
  setWindowTitle("Scenegraph");
  m_pSceneDocument = pDocument;

  m_pStack = new QStackedWidget(this);
  m_pStack->setObjectName("QStackedWidget");
  m_pStack->setContentsMargins(0, 0, 0, 0);
  m_pStack->layout()->setContentsMargins(0, 0, 0, 0);
  setWidget(m_pStack);

  auto pCustomModel = CreateSceneTreeModel(pDocument);
  m_pMainGameObjectWidget = new ezQtGameObjectWidget(this, pDocument, "EditorPluginScene_ScenegraphContextMenu", std::move(pCustomModel), pDocument->GetSceneSelectionManager());
  m_LayerWidgets[pDocument->GetGuid()] = m_pMainGameObjectWidget;
  m_pStack->addWidget(m_pMainGameObjectWidget);

  pDocument->m_LayerEvents.AddEventHandler(ezMakeDelegate(&ezQtScenegraphPanel::LayerEventHandler, this), m_LayerEventUnsubscriber);
  ezHybridArray<ezSceneDocument*, 16> layers;
  pDocument->GetLoadedLayers(layers);
  for (ezSceneDocument* pLayer : layers)
  {
    if (pLayer != pDocument)
      LayerLoaded(pLayer->GetGuid());
  }
  ActiveLayerChanged(pDocument->GetActiveLayer());
}

ezQtScenegraphPanel::~ezQtScenegraphPanel() = default;

void ezQtScenegraphPanel::LayerEventHandler(const ezScene2LayerEvent& e)
{
  switch (e.m_Type)
  {
    case ezScene2LayerEvent::Type::LayerLoaded:
      LayerLoaded(e.m_layerGuid);
      break;
    case ezScene2LayerEvent::Type::LayerUnloaded:
      LayerUnloaded(e.m_layerGuid);
      break;
    case ezScene2LayerEvent::Type::ActiveLayerChanged:
    {
      ActiveLayerChanged(e.m_layerGuid);
    }
    default:
      break;
  }
}

void ezQtScenegraphPanel::LayerLoaded(const ezUuid& layerGuid)
{
  EZ_ASSERT_DEV(!m_LayerWidgets.Contains(layerGuid), "LayerLoaded was fired twice for the same layer.");

  auto pScene2 = static_cast<ezScene2Document*>(m_pSceneDocument);
  auto pLayer = pScene2->GetLayerDocument(layerGuid);
  auto pCustomModel = CreateGameObjectTreeModel(pLayer);
  m_pMainGameObjectWidget = new ezQtGameObjectWidget(this, pLayer, "EditorPluginScene_ScenegraphContextMenu", std::move(pCustomModel));
  m_LayerWidgets[layerGuid] = m_pMainGameObjectWidget;
  m_pStack->addWidget(m_pMainGameObjectWidget);
  ActiveLayerChanged(pScene2->GetActiveLayer());
}

void ezQtScenegraphPanel::LayerUnloaded(const ezUuid& layerGuid)
{
  EZ_ASSERT_DEV(m_LayerWidgets.Contains(layerGuid), "LayerUnloaded was fired without the layer being loaded first.");

  ezQtGameObjectWidget* pWidget = m_LayerWidgets[layerGuid];
  m_pStack->removeWidget(pWidget);
  m_LayerWidgets.Remove(layerGuid);
  delete pWidget;

  auto pScene2 = static_cast<ezScene2Document*>(m_pSceneDocument);
  ActiveLayerChanged(pScene2->GetActiveLayer());
}

void ezQtScenegraphPanel::ActiveLayerChanged(const ezUuid& layerGuid)
{
  // migrate the search filter text to the other layer
  if (ezQtGameObjectWidget* pPrev = qobject_cast<ezQtGameObjectWidget*>(m_pStack->currentWidget()))
  {
    QString sText = pPrev->GetFilterWidget().text();
    m_LayerWidgets[layerGuid]->GetFilterWidget().setText(sText);
  }

  m_pStack->setCurrentWidget(m_LayerWidgets[layerGuid]);
}
