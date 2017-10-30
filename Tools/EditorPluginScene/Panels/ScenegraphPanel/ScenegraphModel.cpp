#include <PCH.h>
#include <EditorPluginScene/Panels/ScenegraphPanel/ScenegraphModel.moc.h>
#include <Foundation/Reflection/Implementation/StaticRTTI.h>
#include <Core/World/GameObject.h>
#include <EditorFramework/Document/GameObjectDocument.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <Foundation/Strings/TranslationLookup.h>

ezQtScenegraphModel::ezQtScenegraphModel(ezSceneDocument* pDocument)
  : ezQtGameObjectModel(pDocument, "Children")
{
  m_pSceneDocument = pDocument;
}

ezQtScenegraphModel::~ezQtScenegraphModel()
{
}

QVariant ezQtScenegraphModel::data(const QModelIndex &index, int role) const
{
  const ezDocumentObject* pObject = (const ezDocumentObject*)index.internalPointer();

  return ezQtGameObjectModel::data(index, role);
}

bool ezQtScenegraphModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
  return ezQtGameObjectModel::setData(index, value, role);
}

