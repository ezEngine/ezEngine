#include <EditorPluginScenePCH.h>

#include <EditorPluginScene/Panels/LayerPanel/LayerAdapter.moc.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <EditorPluginScene/Scene/Scene2Document.h>

ezQtLayerAdapter::ezQtLayerAdapter(ezScene2Document* pDocument)
  : ezQtDocumentTreeModelAdapter(pDocument->GetSceneObjectManager(), ezGetStaticRTTI<ezSceneLayer>(), nullptr)
{
  m_pSceneDocument = pDocument;
  m_pSceneDocument->m_LayerEvents.AddEventHandler(
    ezMakeDelegate(&ezQtLayerAdapter::LayerEventHandler, this), m_LayerEventUnsubscriber);
}

ezQtLayerAdapter::~ezQtLayerAdapter()
{
  m_LayerEventUnsubscriber.Unsubscribe();
}

QVariant ezQtLayerAdapter::data(const ezDocumentObject* pObject, int row, int column, int role) const
{
  switch (role)
  {
    case Qt::DisplayRole:
    {
      ezObjectAccessorBase* pAccessor = m_pSceneDocument->GetSceneObjectAccessor();
      ezUuid layerGuid = pAccessor->Get<ezUuid>(pObject, "Layer");
      // Use curator to get name in case the layer is unloaded and there is no document to query.
      const ezAssetCurator::ezLockedSubAsset subAsset = ezAssetCurator::GetSingleton()->GetSubAsset(layerGuid);
      if (subAsset.isValid())
      {
        ezStringBuilder sName = subAsset->GetName();
        QString sQtName = QString::fromUtf8(sName.GetData());
        if (ezSceneDocument* pLayer = m_pSceneDocument->GetLayerDocument(layerGuid))
        {
          if (pLayer->IsModified())
          {
            sQtName += "*";
          }
        }
        return sQtName;
      }
      else
      {
        return QStringLiteral("Layer guid not found");
      }
    }
    break;

    case Qt::DecorationRole:
    {
      return ezQtUiServices::GetCachedIconResource(":/EditorPluginScene/Icons/Layer16.png");
    }
    break;

    case Qt::ToolTipRole:
    {
      return QStringLiteral("A layer tooltip");
    }
    break;
    case Qt::ForegroundRole:
    {
      ezObjectAccessorBase* pAccessor = m_pSceneDocument->GetSceneObjectAccessor();
      ezUuid layerGuid = pAccessor->Get<ezUuid>(pObject, "Layer");
      if (!m_pSceneDocument->IsLayerLoaded(layerGuid))
      {
        return QColor(128, 128, 128);
      }
    }
    break;
  }

  return QVariant();
}

bool ezQtLayerAdapter::setData(const ezDocumentObject* pObject, int row, int column, const QVariant& value, int role) const
{
  return false;
}

void ezQtLayerAdapter::LayerEventHandler(const ezScene2LayerEvent& e)
{
  switch (e.m_Type)
  {
    case ezScene2LayerEvent::Type::LayerUnloaded:
    case ezScene2LayerEvent::Type::LayerLoaded:
    {
      QVector<int> v;
      v.push_back(Qt::DisplayRole);
      v.push_back(Qt::ForegroundRole);
      Q_EMIT dataChanged(m_pSceneDocument->GetLayerObject(e.m_layerGuid), v);
    }
    break;
    default:
      break;
  }
}

 ezQtLayerModel::ezQtLayerModel(ezScene2Document* pDocument)
  : ezQtDocumentTreeModel(pDocument->GetSceneObjectManager(), pDocument->GetSettingsObject()->GetGuid())
  , m_pDocument(pDocument)
{
}

bool ezQtLayerModel::canDropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) const
{
  return ezQtDocumentTreeModel::canDropMimeData(data, action, row, column, parent);
}

bool ezQtLayerModel::dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent)
{
  // We can't execute the drop because the code is not layer-aware and will try to do the operation on the active document.
  ezUuid activeDoc = m_pDocument->GetActiveLayer();
  m_pDocument->SetActiveLayer(m_pDocument->GetGuid()).LogFailure();
  bool res = ezQtDocumentTreeModel::dropMimeData(data, action, row, column, parent);
  m_pDocument->SetActiveLayer(activeDoc).LogFailure();
  return res;
}
