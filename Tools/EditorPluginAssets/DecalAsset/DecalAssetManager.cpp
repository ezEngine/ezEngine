#include <PCH.h>
#include <EditorPluginAssets/DecalAsset/DecalAssetManager.h>
#include <EditorPluginAssets/DecalAsset/DecalAsset.h>
#include <EditorPluginAssets/DecalAsset/DecalAssetWindow.moc.h>
#include <ToolsFoundation/Assets/AssetFileExtensionWhitelist.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <ToolsFoundation/Project/ToolsProject.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Core/Assets/AssetFileHeader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezDecalAssetDocumentManager, 1, ezRTTIDefaultAllocator<ezDecalAssetDocumentManager>);
EZ_END_DYNAMIC_REFLECTED_TYPE

ezDecalAssetDocumentManager::ezDecalAssetDocumentManager()
{
  ezDocumentManager::s_Events.AddEventHandler(ezMakeDelegate(&ezDecalAssetDocumentManager::OnDocumentManagerEvent, this));

  // texture asset source files
  ezAssetFileExtensionWhitelist::AddAssetFileExtension("Image2D", "dds");
  ezAssetFileExtensionWhitelist::AddAssetFileExtension("Image2D", "tga");

  m_AssetDesc.m_bCanCreate = true;
  m_AssetDesc.m_sDocumentTypeName = "Decal Asset";
  m_AssetDesc.m_sFileExtension = "ezDecalAsset";
  m_AssetDesc.m_sIcon = ":/AssetIcons/Decal.png";
  m_AssetDesc.m_pDocumentType = ezGetStaticRTTI<ezDecalAssetDocument>();
  m_AssetDesc.m_pManager = this;
}

ezDecalAssetDocumentManager::~ezDecalAssetDocumentManager()
{
  ezDocumentManager::s_Events.RemoveEventHandler(ezMakeDelegate(&ezDecalAssetDocumentManager::OnDocumentManagerEvent, this));
}


ezBitflags<ezAssetDocumentFlags> ezDecalAssetDocumentManager::GetAssetDocumentTypeFlags(const ezDocumentTypeDescriptor* pDescriptor) const
{
  return ezAssetDocumentFlags::Default;// ezAssetDocumentFlags::SupportsThumbnail | ezAssetDocumentFlags::AutoThumbnailOnTransform;
}


ezString ezDecalAssetDocumentManager::GetAssetTableEntry(const ezSubAsset* pSubAsset, const char* szDataDirectory, const char* szPlatform) const
{
  ezStringBuilder result = GetDecalTexturePath(szPlatform);

  ezStringBuilder sGuid;
  ezConversionUtils::ToString(pSubAsset->m_Data.m_Guid, sGuid);

  result.Append("|", sGuid);

  return result;
}

void ezDecalAssetDocumentManager::OnDocumentManagerEvent(const ezDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
  case ezDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == ezGetStaticRTTI<ezDecalAssetDocument>())
      {
        ezQtDecalAssetDocumentWindow* pDocWnd = new ezQtDecalAssetDocumentWindow(static_cast<ezDecalAssetDocument*>(e.m_pDocument));
      }
    }
    break;
  }
}

ezStatus ezDecalAssetDocumentManager::InternalCreateDocument(const char* szDocumentTypeName, const char* szPath, ezDocument*& out_pDocument)
{
  out_pDocument = new ezDecalAssetDocument(szPath);

  return ezStatus(EZ_SUCCESS);
}

void ezDecalAssetDocumentManager::InternalGetSupportedDocumentTypes(ezDynamicArray<const ezDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_AssetDesc);
}


ezStatus ezDecalAssetDocumentManager::GenerateDecalTexture(const char* szPlatform)
{
  const ezDynamicArray<ezDocument*>& docs = GetAllDocuments();

  ezAssetCurator* pCurator = ezAssetCurator::GetSingleton();
  const auto& allAssets = pCurator->GetKnownSubAssets();

  ezUInt64 uiSettingsHash = 1;

  for (auto it = allAssets->GetIterator(); it.IsValid(); ++it)
  {
    const auto& asset = it.Value();

    if (asset.m_pAssetInfo->m_pManager != this)
      continue;

    uiSettingsHash += asset.m_pAssetInfo->m_Info.m_uiSettingsHash;

    auto guid = it.Key();
  }

  ezStringBuilder decalFile = ezToolsProject::GetSingleton()->GetProjectDirectory(); 
  decalFile.AppendPath("AssetCache", GetDecalTexturePath(szPlatform));

  if (IsDecalTextureUpToDate(decalFile, uiSettingsHash))
    return ezStatus(EZ_SUCCESS);

  {
    ezFileWriter file;
    if (file.Open(decalFile).Failed())
      return ezStatus(ezFmt("Could not write to file {0}", decalFile));

    ezUInt16 uiVersion = ezGetStaticRTTI<ezDecalAssetDocument>()->GetTypeVersion() & 0xFF;
    uiVersion |= (ezGetStaticRTTI<ezDecalAssetProperties>()->GetTypeVersion() & 0xFF) << 8;

    ezAssetFileHeader header;
    header.SetFileHashAndVersion(uiSettingsHash, uiVersion);

    header.Write(file);
  }

  return ezStatus(EZ_SUCCESS);
}

bool ezDecalAssetDocumentManager::IsDecalTextureUpToDate(const char* szDecalFile, ezUInt64 uiSettingsHash) const
{
  ezFileReader file;
  if (file.Open(szDecalFile).Succeeded())
  {
    ezAssetFileHeader header;
    header.Read(file);

    // file still valid
    if (header.GetFileHash() == uiSettingsHash)
      return true;
  }

  return false;
}

ezString ezDecalAssetDocumentManager::GetDecalTexturePath(const char* szPlatform) const
{
  const ezString sPlatform = ezAssetDocumentManager::DetermineFinalTargetPlatform(szPlatform);
  ezStringBuilder result = "Decals";
  GenerateOutputFilename(result, sPlatform, "ezDecal", true);

  return result;
}


