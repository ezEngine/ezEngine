#include <PCH.h>
#include <EditorPluginScene/Panels/ScenegraphPanel/ScenegraphModel.moc.h>
#include <Foundation/Reflection/Implementation/StaticRTTI.h>
#include <Core/World/GameObject.h>
#include <ToolsFoundation/Document/Document.h>
#include <EditorPluginScene/Scene/SceneDocument.h>
#include <EditorFramework/Assets/AssetCurator.h>

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
  case Qt::DisplayRole:
    {
      auto pMeta = m_pSceneDocument->m_ObjectMetaData.BeginReadMetaData(pObject->GetGuid());
      const bool bPrefab = pMeta->m_CreateFromPrefab.IsValid();
      m_pSceneDocument->m_ObjectMetaData.EndReadMetaData();

      QString sName = QString::fromUtf8(pObject->GetTypeAccessor().GetValue(ezToolsReflectionUtils::CreatePropertyPath("Name")).ConvertTo<ezString>().GetData());

      if (bPrefab)
        return QStringLiteral("[") + sName + QStringLiteral("]");

      return sName;
    }
    break;

  case Qt::ToolTipRole:
    {
      auto pMeta = m_pSceneDocument->m_ObjectMetaData.BeginReadMetaData(pObject->GetGuid());
      const ezUuid prefab = pMeta->m_CreateFromPrefab;
      m_pSceneDocument->m_ObjectMetaData.EndReadMetaData();

      if (prefab.IsValid())
      {
        auto pInfo = ezAssetCurator::GetInstance()->GetAssetInfo(prefab);

        if (pInfo)
          return QString::fromUtf8(pInfo->m_sRelativePath);

        return QStringLiteral("Prefab asset could not be found");
      }

    }
    break;

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

  case Qt::ForegroundRole:
    {
      auto pMeta = m_pSceneDocument->m_ObjectMetaData.BeginReadMetaData(pObject->GetGuid());
      const bool bPrefab = pMeta->m_CreateFromPrefab.IsValid();
      m_pSceneDocument->m_ObjectMetaData.EndReadMetaData();

      if (bPrefab)
        return QColor(0, 128, 196);
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


