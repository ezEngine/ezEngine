#include <EditorPluginAssetsPCH.h>

#include <Core/WorldSerializer/ResourceHandleWriter.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorPluginAssets/CollectionAsset/CollectionAsset.h>
#include <EditorPluginAssets/CollectionAsset/CollectionAssetManager.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Texture/Image/Image.h>
#include <ToolsFoundation/Reflection/PhantomRttiManager.h>

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

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezCollectionAssetDocument, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezCollectionAssetDocument::ezCollectionAssetDocument(const char* szDocumentPath)
    : ezSimpleAssetDocument<ezCollectionAssetData>(szDocumentPath, ezAssetDocEngineConnection::None)
{
}

ezStatus ezCollectionAssetDocument::InternalTransformAsset(ezStreamWriter& stream, const char* szOutputTag, const ezPlatformProfile* pAssetProfile,
                                                           const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags)
{
  const ezCollectionAssetData* pProp = GetProperties();

  ezCollectionResourceDescriptor desc;
  ezCollectionEntry entry;

  for (const auto& e : pProp->m_Entries)
  {
    if (e.m_sRedirectionAsset.IsEmpty())
      continue;

    ezAssetCurator::ezLockedSubAsset pInfo = ezAssetCurator::GetSingleton()->FindSubAsset(e.m_sRedirectionAsset);

    if (pInfo == nullptr)
    {
      ezLog::Warning("Asset in Collection is unknown: '{0}'", e.m_sRedirectionAsset);
      continue;
    }

    entry.m_sOptionalNiceLookupName = e.m_sLookupName;
    entry.m_sResourceID = e.m_sRedirectionAsset;
    entry.m_sAssetTypeName = pInfo->m_Data.m_sAssetTypeName;

    desc.m_Resources.PushBack(entry);
  }

  desc.Save(stream);

  return ezStatus(EZ_SUCCESS);
}
