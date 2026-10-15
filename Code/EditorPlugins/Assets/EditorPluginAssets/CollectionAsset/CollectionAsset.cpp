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
    EZ_MEMBER_PROPERTY("Asset", m_sRedirectionAsset)->AddAttributes(new ezAssetBrowserAttribute("", "*", ezDependencyFlags::Package), new ezRequiredAttribute())
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

static bool InsertEntry(ezStringView sID, ezStringView sLookupName, ezMap<ezString, ezCollectionEntry>& inout_found, ezSet<ezString>& inout_visited)
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

  // Scenes are not added to inout_found, so without this, package dependency cycles through scenes (e.g. two scenes that transition into each other) would recurse endlessly.
  // Only known assets are tracked, so that a top-level entry that was already reached as a transitive dependency still reports an unknown asset.
  if (inout_visited.Contains(sID))
    return true;

  inout_visited.Insert(sID);

  // insert item itself
  // Scenes are not resources (they are loaded through ezSceneLoadUtility), so they can't be preloaded.
  // A scene in a collection is a shortcut for everything it references.
  if (pInfo->m_Data.m_sSubAssetsDocumentTypeName != "Scene")
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
      InsertEntry(doc, {}, inout_found, inout_visited);
    }
  }

  return true;
}

ezTransformStatus ezCollectionAssetDocument::InternalTransformAsset(ezStreamWriter& stream, ezStringView sOutputTag, const ezPlatformProfile* pAssetProfile, const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags)
{
  const ezCollectionAssetData* pProp = GetProperties();

  ezMap<ezString, ezCollectionEntry> entries(ezTempAllocatorWrapper::GetAllocator());
  ezSet<ezString> visited(ezTempAllocatorWrapper::GetAllocator());

  for (const auto& e : pProp->m_Entries)
  {
    if (e.m_sRedirectionAsset.IsEmpty())
      continue;

    if (!InsertEntry(e.m_sRedirectionAsset, e.m_sLookupName, entries, visited))
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
