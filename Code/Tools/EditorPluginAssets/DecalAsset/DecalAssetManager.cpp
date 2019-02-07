#include <EditorPluginAssetsPCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorPluginAssets/DecalAsset/DecalAsset.h>
#include <EditorPluginAssets/DecalAsset/DecalAssetManager.h>
#include <EditorPluginAssets/DecalAsset/DecalAssetWindow.moc.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/OSFile.h>
#include <Texture/Utils/TextureAtlasDesc.h>
#include <ToolsFoundation/Assets/AssetFileExtensionWhitelist.h>
#include <ToolsFoundation/Project/ToolsProject.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezDecalAssetDocumentManager, 1, ezRTTIDefaultAllocator<ezDecalAssetDocumentManager>)
  ;
EZ_END_DYNAMIC_REFLECTED_TYPE;

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
  return ezAssetDocumentFlags::SupportsThumbnail;
}


void ezDecalAssetDocumentManager::AddEntriesToAssetTable(
  const char* szDataDirectory, const ezPlatformProfile* pAssetProfile, ezMap<ezString, ezString>& inout_GuidToPath) const
{
  ezStringBuilder projectDir = ezToolsProject::GetSingleton()->GetProjectDirectory();
  projectDir.MakeCleanPath();
  projectDir.Append("/");

  if (projectDir.StartsWith_NoCase(szDataDirectory))
  {
    inout_GuidToPath["{ ProjectDecalAtlas }"] = "PC/Decals.ezTextureAtlas";
  }
}

ezString ezDecalAssetDocumentManager::GetAssetTableEntry(
  const ezSubAsset* pSubAsset, const char* szDataDirectory, const ezPlatformProfile* pAssetProfile) const
{
  // means NO table entry will be written, because for decals we don't need a redirection
  return ezString();
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

ezStatus ezDecalAssetDocumentManager::InternalCreateDocument(
  const char* szDocumentTypeName, const char* szPath, bool bCreateNewDocument, ezDocument*& out_pDocument)
{
  out_pDocument = new ezDecalAssetDocument(szPath);

  return ezStatus(EZ_SUCCESS);
}

void ezDecalAssetDocumentManager::InternalGetSupportedDocumentTypes(
  ezDynamicArray<const ezDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_AssetDesc);
}

ezUInt64 ezDecalAssetDocumentManager::ComputeAssetProfileHashImpl(const ezPlatformProfile* pAssetProfile) const
{
  // don't have any settings yet, but assets that generate profile specific output must not return 0 here
  return 1;
}

ezStatus ezDecalAssetDocumentManager::GenerateDecalTexture(const ezPlatformProfile* pAssetProfile)
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

    uiSettingsHash += asset.m_pAssetInfo->m_Info->m_uiSettingsHash;
  }

  ezStringBuilder decalFile = ezToolsProject::GetSingleton()->GetProjectDirectory();
  decalFile.AppendPath("AssetCache", GetDecalTexturePath(pAssetProfile));

  if (IsDecalTextureUpToDate(decalFile, uiSettingsHash))
    return ezStatus(EZ_SUCCESS);

  ezTextureAtlasCreationDesc atlasDesc;

  // find all decal assets, extract their file information to pass it along to TexConv
  {
    atlasDesc.m_Layers.SetCount(2);
    atlasDesc.m_Layers[0].m_Usage = ezTexConvUsage::Color;
    atlasDesc.m_Layers[1].m_Usage = ezTexConvUsage::NormalMap;

    atlasDesc.m_Items.Reserve(64);

    ezQtEditorApp* pEditorApp = ezQtEditorApp::GetSingleton();
    ezStringBuilder sAbsPath;

    for (auto it = allAssets->GetIterator(); it.IsValid(); ++it)
    {
      const auto& asset = it.Value();

      if (asset.m_pAssetInfo->m_pManager != this)
        continue;

      EZ_LOG_BLOCK("Decal Asset", asset.m_pAssetInfo->m_sDataDirRelativePath.GetData());

      // does the document already exist and is it open ?
      bool bWasOpen = false;
      ezDocument* pDoc = GetDocumentByPath(asset.m_pAssetInfo->m_sAbsolutePath);
      if (pDoc)
        bWasOpen = true;
      else
        pDoc = pEditorApp->OpenDocument(asset.m_pAssetInfo->m_sAbsolutePath, ezDocumentFlags::None);

      if (pDoc == nullptr)
        return ezStatus(ezFmt("Could not open asset document '{0}'", asset.m_pAssetInfo->m_sDataDirRelativePath));

      ezDecalAssetDocument* pDecalAsset = static_cast<ezDecalAssetDocument*>(pDoc);

      {
        auto& item = atlasDesc.m_Items.ExpandAndGetRef();

        // store the GUID as the decal identifier
        ezConversionUtils::ToString(pDecalAsset->GetGuid(), sAbsPath);
        item.m_uiUniqueID = ezHashingUtils::xxHash32(sAbsPath.GetData(), sAbsPath.GetElementCount());

        {
          sAbsPath = pDecalAsset->GetProperties()->m_sBaseColor;
          if (!pEditorApp->MakeDataDirectoryRelativePathAbsolute(sAbsPath))
          {
            return ezStatus(ezFmt("Invalid texture path '{0}'", sAbsPath));
          }

          item.m_sLayerInput[0] = sAbsPath;
        }

        {
          sAbsPath = pDecalAsset->GetProperties()->m_sNormal;
          if (!pEditorApp->MakeDataDirectoryRelativePathAbsolute(sAbsPath))
          {
            return ezStatus(ezFmt("Invalid texture path '{0}'", sAbsPath));
          }

          item.m_sLayerInput[1] = sAbsPath;
        }
      }


      if (!pDoc->HasWindowBeenRequested() && !bWasOpen)
        pDoc->GetDocumentManager()->CloseDocument(pDoc);
    }
  }

  ezAssetFileHeader header;
  {
    ezUInt16 uiVersion = ezGetStaticRTTI<ezDecalAssetDocument>()->GetTypeVersion() & 0xFF;
    uiVersion |= (ezGetStaticRTTI<ezDecalAssetProperties>()->GetTypeVersion() & 0xFF) << 8;

    header.SetFileHashAndVersion(uiSettingsHash, uiVersion);
  }

  ezStatus result;

  // Send information to TexConv to do all the work
  {
    ezStringBuilder texGroupFile = ezToolsProject::GetSingleton()->GetProjectDirectory();
    texGroupFile.AppendPath("AssetCache", GetDecalTexturePath(pAssetProfile));
    texGroupFile.ChangeFileExtension("ezDecalAtlasDesc");

    if (atlasDesc.Save(texGroupFile).Failed())
      return ezStatus(ezFmt("Failed to save texture atlas descriptor file '{0}'", texGroupFile));

    result = RunTexConv(decalFile, texGroupFile, header);
  }

  ezFileStats stat;
  if (ezOSFile::GetFileStats(decalFile, stat).Succeeded() && stat.m_uiFileSize == 0)
  {
    // if the file was touched, but nothing written to it, delete the file
    // might happen if TexConv crashed or had an error
    ezOSFile::DeleteFile(decalFile);
    result.m_Result = EZ_FAILURE;
  }

  return result;
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

ezString ezDecalAssetDocumentManager::GetDecalTexturePath(const ezPlatformProfile* pAssetProfile0) const
{
  const ezPlatformProfile* pAssetProfile = ezAssetDocumentManager::DetermineFinalTargetProfile(pAssetProfile0);
  ezStringBuilder result = "Decals";
  GenerateOutputFilename(result, pAssetProfile, "ezTextureAtlas", true);

  return result;
}

ezStatus ezDecalAssetDocumentManager::RunTexConv(const char* szTargetFile, const char* szInputFile, const ezAssetFileHeader& AssetHeader)
{
  QStringList arguments;
  ezStringBuilder temp;

  // Asset Version
  {
    arguments << "-assetVersion";
    arguments << ezConversionUtils::ToString(AssetHeader.GetFileVersion(), temp).GetData();
  }

  // Asset Hash
  {
    const ezUInt64 uiHash64 = AssetHeader.GetFileHash();
    const ezUInt32 uiHashLow32 = uiHash64 & 0xFFFFFFFF;
    const ezUInt32 uiHashHigh32 = (uiHash64 >> 32) & 0xFFFFFFFF;

    temp.Format("{0}", ezArgU(uiHashLow32, 8, true, 16, true));
    arguments << "-assetHashLow";
    arguments << temp.GetData();

    temp.Format("{0}", ezArgU(uiHashHigh32, 8, true, 16, true));
    arguments << "-assetHashHigh";
    arguments << temp.GetData();
  }


  arguments << "-out";
  arguments << szTargetFile;

  arguments << "-type";
  arguments << "TextureAtlas";

  arguments << "-atlasDesc";
  arguments << QString(szInputFile);

  ezStringBuilder cmd;
  for (ezInt32 i = 0; i < arguments.size(); ++i)
    cmd.Append(" ", arguments[i].toUtf8().data());

  ezLog::Debug("TexConv2.exe{0}", cmd);

  EZ_SUCCEED_OR_RETURN(ezQtEditorApp::GetSingleton()->ExecuteTool("TexConv2.exe", arguments, 60, ezLog::GetThreadLocalLogSystem()));

  return ezStatus(EZ_SUCCESS);
}
