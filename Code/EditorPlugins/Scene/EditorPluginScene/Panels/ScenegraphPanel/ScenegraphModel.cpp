#include <EditorPluginScenePCH.h>

#include <EditorPluginScene/Panels/ScenegraphPanel/ScenegraphModel.moc.h>

ezQtScenegraphModel::ezQtScenegraphModel(ezSceneDocument* pDocument, const ezUuid& root)
  : ezQtGameObjectModel(pDocument, root)
{
  m_pSceneDocument = pDocument;
}

ezQtScenegraphModel::~ezQtScenegraphModel() {}
