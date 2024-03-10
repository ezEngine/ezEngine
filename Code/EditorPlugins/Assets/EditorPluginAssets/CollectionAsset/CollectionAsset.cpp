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
    EZ_MEMBER_PROPERTY("Asset", m_sRedirectionAsset)->AddAttributes(new ezAssetBrowserAttribute("", "*", ezDependencyFlags::Package))
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

ezCollectionAssetDocument::ezCollectionAssetDocument(ezStringView sDocumentPath)
  : ezSimpleAssetDocument<ezCollectionAssetData>(sDocumentPath, ezAssetDocEngineConnection::None)
{
}

static bool InsertEntry(ezStringView sID, ezStringView sLookupName, ezMap<ezString, ezCollectionEntry>& inout_found)
{
  auto it = inout_found.Find(sID);

  if (it.IsValid())
  {
    if (!sLookupName.IsEmpty())
    {
      it.Value().m_sOptionalNiceLookupName = sLookupName;
    }

    return true;
  }

  ezStringBuilder tmp;
  ezAssetCurator::ezLockedSubAsset pInfo = ezAssetCurator::GetSingleton()->FindSubAsset(sID.GetData(tmp));

  if (pInfo == nullptr)
  {
    // this happens for non-asset types (e.g. 'xyz.color' and other non-asset file types)
    // these are benign and can just be skipped
    return false;
  }

  // insert item itself
  {
    ezCollectionEntry& entry = inout_found[sID];
    entry.m_sOptionalNiceLookupName = sLookupName;
    entry.m_sResourceID = sID;
    entry.m_sAssetTypeName = pInfo->m_Data.m_sSubAssetsDocumentTypeName;
  }

  // insert dependencies
  {
    const ezAssetDocumentInfo* pDocInfo = pInfo->m_pAssetInfo->m_Info.Borrow();

    for (const ezString& doc : pDocInfo->m_PackageDependencies)
    {
      // ignore return value, we are only interested in top-level information
      InsertEntry(doc, {}, inout_found);
    }
  }

  return true;
}

ezTransformStatus ezCollectionAssetDocument::InternalTransformAsset(ezStreamWriter& stream, ezStringView sOutputTag, const ezPlatformProfile* pAssetProfile, const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags)
{
  const ezCollectionAssetData* pProp = GetProperties();

  ezMap<ezString, ezCollectionEntry> entries;

  for (const auto& e : pProp->m_Entries)
  {
    if (e.m_sRedirectionAsset.IsEmpty())
      continue;

    if (!InsertEntry(e.m_sRedirectionAsset, e.m_sLookupName, entries))
    {
      // this should be treated as an error for top-level references, since they are manually added (in contrast to the transitive dependencies)
      return ezStatus(ezFmt("Asset in Collection is unknown: '{0}'", e.m_sRedirectionAsset));
    }
  }

  ezCollectionResourceDescriptor desc;

  for (auto it : entries)
  {
    desc.m_Resources.PushBack(it.Value());
  }

  desc.Save(stream);

  return ezStatus(EZ_SUCCESS);
}
