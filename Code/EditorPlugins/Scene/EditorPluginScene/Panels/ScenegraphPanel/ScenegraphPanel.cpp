#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorPluginScene/Panels/ScenegraphPanel/ScenegraphModel.moc.h>
#include <EditorPluginScene/Panels/ScenegraphPanel/ScenegraphPanel.moc.h>

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
  : ezQtGameObjectPanel(pParent, pDocument, "EditorPluginScene_ScenegraphContextMenu", CreateSceneTreeModel(pDocument))
{
  setObjectName("ScenegraphPanel");
  setWindowTitle("Scenegraph");
  m_pSceneDocument = pDocument;
}

ezQtScenegraphPanel::~ezQtScenegraphPanel() {}
