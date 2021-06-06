#include <EditorPluginScenePCH.h>

#include <EditorPluginScene/Panels/ScenegraphPanel/ScenegraphModel.moc.h>
#include <EditorPluginScene/Panels/ScenegraphPanel/ScenegraphPanel.moc.h>

#include <QStackedWidget>
#include <QLayout>

namespace
{
  std::unique_ptr<ezQtDocumentTreeModel> CreateSceneTreeModel(ezSceneDocument* pDocument)
  {
    std::unique_ptr<ezQtDocumentTreeModel> pModel(new ezQtScenegraphModel(pDocument));
    pModel->AddAdapter(new ezQtDummyAdapter(pDocument->GetObjectManager(), ezGetStaticRTTI<ezDocumentRoot>(), "Children"));
    pModel->AddAdapter(new ezQtGameObjectAdapter(pDocument));
    return std::move(pModel);
  }
} // namespace

ezQtScenegraphPanel::ezQtScenegraphPanel(QWidget* pParent, ezSceneDocument* pDocument)
  : ezQtDocumentPanel(pParent)
{
  setObjectName("ScenegraphPanel");
  setWindowTitle("Scenegraph");
  m_pSceneDocument = pDocument;

  m_pStack = new QStackedWidget(this);
  m_pStack->setContentsMargins(0, 0, 0, 0);
  m_pStack->layout()->setContentsMargins(0, 0, 0, 0);
  setWidget(m_pStack);

  auto pCustomModel = CreateSceneTreeModel(pDocument);
  m_pMainGameObjectWidget = new ezQtGameObjectWidget(this, pDocument, "EditorPluginScene_ScenegraphContextMenu", std::move(pCustomModel));
  m_pStack->addWidget(m_pMainGameObjectWidget);
}

ezQtScenegraphPanel::~ezQtScenegraphPanel() {}
