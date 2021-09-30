#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorPluginScene/Panels/ScenegraphPanel/ScenegraphModel.moc.h>

ezQtScenegraphModel::ezQtScenegraphModel(const ezDocumentObjectManager* pObjectManager, const ezUuid& root)
  : ezQtGameObjectModel(pObjectManager, root)
{
}

ezQtScenegraphModel::~ezQtScenegraphModel() {}
