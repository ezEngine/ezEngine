#include <PCH.h>
#include <EditorPluginScene/Panels/ScenegraphPanel/ScenegraphModel.moc.h>
#include <Foundation/Reflection/Implementation/StaticRTTI.h>
#include <Core/World/GameObject.h>
#include <ToolsFoundation/Document/Document.h>
#include <EditorPluginScene/Scene/SceneDocument.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <CoreUtils/Localization/TranslationLookup.h>

ezQtScenegraphModel::ezQtScenegraphModel(ezSceneDocument* pDocument)
  : ezQtDocumentTreeModel(pDocument->GetObjectManager(), ezGetStaticRTTI<ezGameObject>(), "Children")
{
  m_pSceneDocument = pDocument;

  m_pSceneDocument->m_SceneObjectMetaData.m_DataModifiedEvent.AddEventHandler(ezMakeDelegate(&ezQtScenegraphModel::SceneObjectMetaDataEventHandler, this));
  m_pSceneDocument->m_DocumentObjectMetaData.m_DataModifiedEvent.AddEventHandler(ezMakeDelegate(&ezQtScenegraphModel::DocumentObjectMetaDataEventHandler, this));
}

ezQtScenegraphModel::~ezQtScenegraphModel()
{
  m_pSceneDocument->m_SceneObjectMetaData.m_DataModifiedEvent.RemoveEventHandler(ezMakeDelegate(&ezQtScenegraphModel::SceneObjectMetaDataEventHandler, this));
  m_pSceneDocument->m_DocumentObjectMetaData.m_DataModifiedEvent.RemoveEventHandler(ezMakeDelegate(&ezQtScenegraphModel::DocumentObjectMetaDataEventHandler, this));
}

void ezQtScenegraphModel::DetermineNodeName(const ezDocumentObject* pObject, const ezUuid& prefabGuid, ezStringBuilder& out_Result, QIcon& icon) const
{
  // tries to find a good name for a node by looking at the attached components and their properties

  bool bHasIcon = false;

  if (prefabGuid.IsValid())
  {
    auto pInfo = ezAssetCurator::GetSingleton()->GetAssetInfo2(prefabGuid);

    if (pInfo)
    {
      ezStringBuilder sPath = pInfo->m_sDataDirRelativePath;
      sPath = sPath.GetFileName();

      out_Result.Set("Prefab: ", sPath);
    }
    else
      out_Result = "Prefab: Invalid Asset";
  }

  bool bHasChildren = false;

  ezHybridArray<ezVariant, 16> values;
  ezStringBuilder componentProp = "Components";
  pObject->GetTypeAccessor().GetValues(componentProp, values);
  for (ezVariant& value : values)
  {
    auto pChild = m_pSceneDocument->GetObjectManager()->GetObject(value.Get<ezUuid>());

    // search for components
    if (pChild->GetTypeAccessor().GetType()->IsDerivedFrom<ezComponent>())
    {
      // take the first components name
      
      if (!bHasIcon)
      {
        bHasIcon = true;

        ezStringBuilder sIconName;
        sIconName.Set(":/TypeIcons/", pChild->GetTypeAccessor().GetType()->GetTypeName());
        icon = ezQtUiServices::GetCachedIconResource(sIconName.GetData());
      }

      if (out_Result.IsEmpty())
      {
        // try to translate the component name, that will typically make it a nice clean name already
        out_Result = ezTranslate(pChild->GetTypeAccessor().GetType()->GetTypeName());

        // if no translation is available, clean up the component name in a simple way
        if (out_Result.EndsWith_NoCase("Component"))
          out_Result.Shrink(0, 9);
        if (out_Result.StartsWith("ez"))
          out_Result.Shrink(2, 0);
      }

      if (prefabGuid.IsValid())
        continue;

      const auto& properties = pChild->GetTypeAccessor().GetType()->GetProperties();

      for (auto pProperty : properties)
      {
        // search for string properties that also have an asset browser property -> they reference an asset, so this is most likely the most relevant property
        if (pProperty->GetCategory() == ezPropertyCategory::Member && 
            (pProperty->GetSpecificType() == ezGetStaticRTTI<const char*>() ||
             pProperty->GetSpecificType() == ezGetStaticRTTI<ezString>()) &&
            pProperty->GetAttributeByType<ezAssetBrowserAttribute>() != nullptr)
        {
          ezStringBuilder sValue = pChild->GetTypeAccessor().GetValue(pProperty->GetPropertyName()).ConvertTo<ezString>();

          // if the property is a full asset guid reference, convert it to a file name
          if (ezConversionUtils::IsStringUuid(sValue))
          {
            const ezUuid AssetGuid = ezConversionUtils::ConvertStringToUuid(sValue);

            auto pAsset = ezAssetCurator::GetSingleton()->GetAssetInfo2(AssetGuid);

            if (pAsset)
              sValue = pAsset->m_sDataDirRelativePath;
            else
              sValue = "<unknown>";
          }

          // only use the file name for our display
          sValue = sValue.GetFileName();

          if (!sValue.IsEmpty())
            out_Result.Append(": ", sValue);

          return;
        }
      }
    }
    else
    {
      // must be ezGameObject children
      bHasChildren = true;
    }
  }

  if (!out_Result.IsEmpty())
    return;

  if (bHasChildren)
    out_Result = "Group";
  else
    out_Result = "Entity";
}

QVariant ezQtScenegraphModel::data(const QModelIndex &index, int role) const
{
  const ezDocumentObject* pObject = (const ezDocumentObject*)index.internalPointer();

  switch (role)
  {
  case Qt::DisplayRole:
    {
      ezStringBuilder sName = pObject->GetTypeAccessor().GetValue("Name").ConvertTo<ezString>();

      auto pMetaScene = m_pSceneDocument->m_SceneObjectMetaData.BeginReadMetaData(pObject->GetGuid());
      auto pMetaDoc = m_pSceneDocument->m_DocumentObjectMetaData.BeginReadMetaData(pObject->GetGuid());
      const ezUuid prefabGuid = pMetaDoc->m_CreateFromPrefab;

      if (sName.IsEmpty())
        sName = pMetaScene->m_CachedNodeName;

      m_pSceneDocument->m_SceneObjectMetaData.EndReadMetaData();
      m_pSceneDocument->m_DocumentObjectMetaData.EndReadMetaData();

      if (sName.IsEmpty())
      {
        // the cached node name is only determined once
        // after that only a node rename (EditRole) will currently trigger a cache cleaning and thus a reevaluation
        // this is to prevent excessive re-computation of the name, which is quite involved

        QIcon icon;
        DetermineNodeName(pObject, prefabGuid, sName, icon);

        auto pMetaWrite = m_pSceneDocument->m_SceneObjectMetaData.BeginModifyMetaData(pObject->GetGuid());
        pMetaWrite->m_CachedNodeName = sName;
        pMetaWrite->m_Icon = icon;
        m_pSceneDocument->m_SceneObjectMetaData.EndModifyMetaData(0); // no need to broadcast this change
      }

      const QString sQtName = QString::fromUtf8(sName.GetData());

      if (prefabGuid.IsValid())
        return QStringLiteral("[") + sQtName + QStringLiteral("]");

      return sQtName;
    }
    break;

  case Qt::DecorationRole:
    {
      auto pMeta = m_pSceneDocument->m_SceneObjectMetaData.BeginReadMetaData(pObject->GetGuid());
      QIcon icon = pMeta->m_Icon;
      m_pSceneDocument->m_SceneObjectMetaData.EndReadMetaData();

      return icon;
    }
    break;

  case Qt::EditRole:
    {
      ezStringBuilder sName = pObject->GetTypeAccessor().GetValue("Name").ConvertTo<ezString>();

      if (sName.IsEmpty())
      {
        auto pMeta = m_pSceneDocument->m_SceneObjectMetaData.BeginReadMetaData(pObject->GetGuid());
        sName = pMeta->m_CachedNodeName;
        m_pSceneDocument->m_SceneObjectMetaData.EndReadMetaData();
      }

      return QString::fromUtf8(sName.GetData());
    }
    break;

  case Qt::ToolTipRole:
    {
      auto pMeta = m_pSceneDocument->m_DocumentObjectMetaData.BeginReadMetaData(pObject->GetGuid());
      const ezUuid prefab = pMeta->m_CreateFromPrefab;
      m_pSceneDocument->m_DocumentObjectMetaData.EndReadMetaData();

      if (prefab.IsValid())
      {
        auto pInfo = ezAssetCurator::GetSingleton()->GetAssetInfo2(prefab);

        if (pInfo)
          return QString::fromUtf8(pInfo->m_sDataDirRelativePath);

        return QStringLiteral("Prefab asset could not be found");
      }

    }
    break;

  case Qt::FontRole:
    {
      auto pMeta = m_pSceneDocument->m_DocumentObjectMetaData.BeginReadMetaData(pObject->GetGuid());
      const bool bHidden = pMeta->m_bHidden;
      m_pSceneDocument->m_DocumentObjectMetaData.EndReadMetaData();

      const bool bHasName = !pObject->GetTypeAccessor().GetValue("Name").ConvertTo<ezString>().IsEmpty();

      if (bHidden || bHasName)
      {
        QFont font;

        if (bHidden)
          font.setStrikeOut(true);
        if (bHasName)
          font.setBold(true);

        return font;
      }
    }
    break;

  case Qt::ForegroundRole:
    {
      ezStringBuilder sName = pObject->GetTypeAccessor().GetValue("Name").ConvertTo<ezString>();
      
      auto pMeta = m_pSceneDocument->m_DocumentObjectMetaData.BeginReadMetaData(pObject->GetGuid());
      const bool bPrefab = pMeta->m_CreateFromPrefab.IsValid();
      m_pSceneDocument->m_DocumentObjectMetaData.EndReadMetaData();

      if (bPrefab)
      {
        return QColor(0, 128, 196);
      }

      if (sName.IsEmpty())
      {
        // uses an auto generated name
        return QColor(128, 128, 128);
      }

    }
    break;
  }

  return ezQtDocumentTreeModel::data(index, role);
}

bool ezQtScenegraphModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
  if (role == Qt::EditRole)
  {
    const ezDocumentObject* pObject = (const ezDocumentObject*)index.internalPointer();
 
    auto pMetaWrite = m_pSceneDocument->m_SceneObjectMetaData.BeginModifyMetaData(pObject->GetGuid());

    ezStringBuilder sNewValue = value.toString().toUtf8().data();

    const ezStringBuilder sOldValue = pMetaWrite->m_CachedNodeName;

    pMetaWrite->m_CachedNodeName.Clear();
    m_pSceneDocument->m_SceneObjectMetaData.EndModifyMetaData(0); // no need to broadcast this change
    
    if (sOldValue == sNewValue && !sOldValue.IsEmpty())
      return false;

    sNewValue.Trim("[]{}() \t\r"); // forbid these

    return ezQtDocumentTreeModel::setData(index, QString::fromUtf8(sNewValue.GetData()), role);
  }

  return false;
}


void ezQtScenegraphModel::TreeEventHandler(const ezDocumentObjectStructureEvent& e)
{
  ezQtDocumentTreeModel::TreeEventHandler(e);

  switch (e.m_EventType)
  {
  case ezDocumentObjectStructureEvent::Type::AfterObjectMoved2:
  case ezDocumentObjectStructureEvent::Type::AfterObjectAdded:
  case ezDocumentObjectStructureEvent::Type::AfterObjectRemoved:
    if (e.m_sParentProperty == "Components")
    {
      if (e.m_pPreviousParent != nullptr)
      {
        auto pMeta = m_pSceneDocument->m_SceneObjectMetaData.BeginModifyMetaData(e.m_pPreviousParent->GetGuid());
        pMeta->m_CachedNodeName.Clear();
        m_pSceneDocument->m_SceneObjectMetaData.EndModifyMetaData(ezSceneObjectMetaData::CachedName);

        auto idx = ComputeModelIndex(e.m_pPreviousParent);
        emit dataChanged(idx, idx);
      }

      if (e.m_pNewParent != nullptr)
      {
        auto pMeta = m_pSceneDocument->m_SceneObjectMetaData.BeginModifyMetaData(e.m_pNewParent->GetGuid());
        pMeta->m_CachedNodeName.Clear();
        m_pSceneDocument->m_SceneObjectMetaData.EndModifyMetaData(ezSceneObjectMetaData::CachedName);

        auto idx = ComputeModelIndex(e.m_pNewParent);
        emit dataChanged(idx, idx);
      }
    }
    break;
  }

  
}

void ezQtScenegraphModel::DocumentObjectMetaDataEventHandler(const ezObjectMetaData<ezUuid, ezDocumentObjectMetaData>::EventData& e)
{
  if ((e.m_uiModifiedFlags & (ezDocumentObjectMetaData::HiddenFlag | ezDocumentObjectMetaData::PrefabFlag)) == 0)
    return;

  auto pObject = m_pSceneDocument->GetObjectManager()->GetObject(e.m_ObjectKey);
  
  if (pObject == nullptr)
  {
    // The object was destroyed due to a clear of the redo queue, i.e. it is not contained in the scene anymore.
    // So we of course won't find it in the model and thus can skip it.
    return;
  }

  // ignore all components etc.
  if (!pObject->GetTypeAccessor().GetType()->IsDerivedFrom<ezGameObject>())
    return;

  auto index = ComputeModelIndex(pObject);

  QVector<int> v;
  v.push_back(Qt::FontRole);
  dataChanged(index, index, v);
}

void ezQtScenegraphModel::SceneObjectMetaDataEventHandler(const ezObjectMetaData<ezUuid, ezSceneObjectMetaData>::EventData& e)
{
  if (e.m_uiModifiedFlags == 0)
    return;

    auto pObject = m_pSceneDocument->GetObjectManager()->GetObject(e.m_ObjectKey);

  if (pObject == nullptr)
  {
    // The object was destroyed due to a clear of the redo queue, i.e. it is not contained in the scene anymore.
    // So we of course won't find it in the model and thus can skip it.
    return;
  }

  // ignore all components etc.
  if (!pObject->GetTypeAccessor().GetType()->IsDerivedFrom<ezGameObject>())
    return;

  auto index = ComputeModelIndex(pObject);

  QVector<int> v;
  v.push_back(Qt::FontRole);
  dataChanged(index, index, v);
}



