#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorPluginScene/Panels/ScenegraphPanel/ScenegraphModel.moc.h>

ezQtScenegraphModel::ezQtScenegraphModel(ezSceneDocument* pDocument)
  : ezQtGameObjectModel(pDocument)
{
  m_pSceneDocument = pDocument;
}

ezQtScenegraphModel::~ezQtScenegraphModel() {}
