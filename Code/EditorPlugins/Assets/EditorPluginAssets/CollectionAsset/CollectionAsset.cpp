#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetDocumentInfo.h>
#include <EditorPluginAssets/CollectionAsset/CollectionAsset.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezCollectionAssetEntry, 1, ezRTTIDefaultAllocator<ezCollectionAssetEntry>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Name", m_sLookupName),
    EZ_MEMBER_PROPERTY("Asset", m_sRedirectionAsset)->AddAttributes(new ezAssetBrowserAttribute(""))
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezCollectionAssetData, 1, ezRTTIDefaultAllocator<ezCollectionAssetData>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ARRAY_MEMBER_PROPERTY("Entries", m_Entries),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezCollectionAssetDocument, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezCollectionAssetDocument::ezCollectionAssetDocument(const char* szDocumentPath)
  : ezSimpleAssetDocument<ezCollectionAssetData>(szDocumentPath, ezAssetDocEngineConnection::None)
{
}

static void InsertEntry(ezStringView sID, ezStringView sLookupName, ezMap<ezString, ezCollectionEntry>& inout_Found)
{
  auto it = inout_Found.Find(sID);

  if (it.IsValid())
  {
    if (!sLookupName.IsEmpty())
    {
      it.Value().m_sOptionalNiceLookupName = sLookupName;
    }

    return;
  }

  ezStringBuilder tmp;
  ezAssetCurator::ezLockedSubAsset pInfo = ezAssetCurator::GetSingleton()->FindSubAsset(sID.GetData(tmp));

  if (pInfo == nullptr)
  {
    ezLog::Warning("Asset in Collection is unknown: '{0}'", sID);
    return;
  }

  // insert item itself
  {
    ezCollectionEntry& entry = inout_Found[sID];
    entry.m_sOptionalNiceLookupName = sLookupName;
    entry.m_sResourceID = sID;
    entry.m_sAssetTypeName = pInfo->m_Data.m_sSubAssetsDocumentTypeName;
  }

  // insert dependencies
  {
    const ezAssetDocumentInfo* pDocInfo = pInfo->m_pAssetInfo->m_Info.Borrow();

    for (const ezString& doc : pDocInfo->m_AssetTransformDependencies)
    {
      InsertEntry(doc, {}, inout_Found);
    }

    for (const ezString& doc : pDocInfo->m_RuntimeDependencies)
    {
      InsertEntry(doc, {}, inout_Found);
    }
  }
}

void ezCollectionAssetDocument::UpdateAssetDocumentInfo(ezAssetDocumentInfo* pInfo) const
{
  // TODO: why are collections not marked as needs-transform, out of the box, when a dependency changes ?

  SUPER::UpdateAssetDocumentInfo(pInfo);

  const ezCollectionAssetData* pProp = GetProperties();

  ezMap<ezString, ezCollectionEntry> entries;

  for (const auto& e : pProp->m_Entries)
  {
    if (e.m_sRedirectionAsset.IsEmpty())
      continue;

    InsertEntry(e.m_sRedirectionAsset, e.m_sLookupName, entries);
  }

  for (auto it : entries)
  {
    pInfo->m_AssetTransformDependencies.Insert(it.Value().m_sResourceID);
  }
}

ezTransformStatus ezCollectionAssetDocument::InternalTransformAsset(ezStreamWriter& stream, const char* szOutputTag, const ezPlatformProfile* pAssetProfile, const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags)
{
  const ezCollectionAssetData* pProp = GetProperties();

  ezMap<ezString, ezCollectionEntry> entries;

  for (const auto& e : pProp->m_Entries)
  {
    if (e.m_sRedirectionAsset.IsEmpty())
      continue;

    InsertEntry(e.m_sRedirectionAsset, e.m_sLookupName, entries);
  }

  ezCollectionResourceDescriptor desc;

  for (auto it : entries)
  {
    desc.m_Resources.PushBack(it.Value());
  }

  desc.Save(stream);

  return ezStatus(EZ_SUCCESS);
}
