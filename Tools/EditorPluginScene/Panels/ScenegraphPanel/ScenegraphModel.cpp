#include <PCH.h>
#include <EditorPluginScene/Panels/ScenegraphPanel/ScenegraphModel.moc.h>
#include <Foundation/Reflection/Implementation/StaticRTTI.h>
#include <Core/World/GameObject.h>
#include <ToolsFoundation/Document/Document.h>
#include <EditorPluginScene/Scene/SceneDocument.h>

ezQtScenegraphModel::ezQtScenegraphModel(ezSceneDocument* pDocument)
  : ezQtDocumentTreeModel(pDocument->GetObjectManager(), ezGetStaticRTTI<ezGameObject>(), "Children")
{
  m_pSceneDocument = pDocument;

  m_pSceneDocument->m_ObjectMetaData.m_DataModifiedEvent.AddEventHandler(ezMakeDelegate(&ezQtScenegraphModel::ObjectMetaDataEventHandler, this));
}

ezQtScenegraphModel::~ezQtScenegraphModel()
{
  m_pSceneDocument->m_ObjectMetaData.m_DataModifiedEvent.RemoveEventHandler(ezMakeDelegate(&ezQtScenegraphModel::ObjectMetaDataEventHandler, this));
}

QVariant ezQtScenegraphModel::data(const QModelIndex &index, int role) const
{
  const ezDocumentObject* pObject = (const ezDocumentObject*)index.internalPointer();

  switch (role)
  {
  case Qt::FontRole: // probably should use something else for displaying hidden objects
    {
      auto pMeta = m_pSceneDocument->m_ObjectMetaData.BeginReadMetaData(pObject->GetGuid());
      const bool bHidden = pMeta->m_bHidden;
      m_pSceneDocument->m_ObjectMetaData.EndReadMetaData();

      if (bHidden)
      {
        QFont font;
        font.setStrikeOut(true);
        return font;
      }
    }
    break;
  }

  return ezQtDocumentTreeModel::data(index, role);
}

void ezQtScenegraphModel::ObjectMetaDataEventHandler(const ezObjectMetaData<ezUuid, ezSceneObjectMetaData>::EventData& e)
{
  auto pObject = m_pSceneDocument->GetObjectManager()->GetObject(e.m_ObjectKey);

  auto index = ComputeModelIndex(pObject);

  QVector<int> v;
  v.push_back(Qt::FontRole);
  dataChanged(index, index, v);
}


