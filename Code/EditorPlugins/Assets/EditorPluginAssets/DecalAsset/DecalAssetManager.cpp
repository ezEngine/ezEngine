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

#include <RendererCore/../../../Data/Base/Shaders/Common/LightData.h>

const char* ToCompressionMode(ezTexConvCompressionMode::Enum mode);
const char* ToMipmapMode(ezTexConvMipmapMode::Enum mode);

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezDecalAssetDocumentManager, 1, ezRTTIDefaultAllocator<ezDecalAssetDocumentManager>);
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezDecalAssetDocumentManager::ezDecalAssetDocumentManager()
{
  ezDocumentManager::s_Events.AddEventHandler(ezMakeDelegate(&ezDecalAssetDocumentManager::OnDocumentManagerEvent, this));

  // texture asset source files
  ezAssetFileExtensionWhitelist::AddAssetFileExtension("Image2D", "dds");
  ezAssetFileExtensionWhitelist::AddAssetFileExtension("Image2D", "tga");

  m_DocTypeDesc.m_sDocumentTypeName = "Decal";
  m_DocTypeDesc.m_sFileExtension = "ezDecalAsset";
  m_DocTypeDesc.m_sIcon = ":/AssetIcons/Decal.png";
  m_DocTypeDesc.m_pDocumentType = ezGetStaticRTTI<ezDecalAssetDocument>();
  m_DocTypeDesc.m_pManager = this;

  m_DocTypeDesc.m_sResourceFileExtension = "ezDecalStub";
  m_DocTypeDesc.m_AssetDocumentFlags = ezAssetDocumentFlags::SupportsThumbnail;
}

ezDecalAssetDocumentManager::~ezDecalAssetDocumentManager()
{
  ezDocumentManager::s_Events.RemoveEventHandler(ezMakeDelegate(&ezDecalAssetDocumentManager::OnDocumentManagerEvent, this));
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

void ezDecalAssetDocumentManager::InternalCreateDocument(const char* szDocumentTypeName, const char* szPath, bool bCreateNewDocument, ezDocument*& out_pDocument)
{
  out_pDocument = new ezDecalAssetDocument(szPath);
}

void ezDecalAssetDocumentManager::InternalGetSupportedDocumentTypes(ezDynamicArray<const ezDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
}

ezUInt64 ezDecalAssetDocumentManager::ComputeAssetProfileHashImpl(const ezPlatformProfile* pAssetProfile) const
{
  // don't have any settings yet, but assets that generate profile specific output must not return 0 here
  return 1;
}

ezStatus ezDecalAssetDocumentManager::GenerateDecalTexture(const ezPlatformProfile* pAssetProfile)
{
  ezAssetCurator* pCurator = ezAssetCurator::GetSingleton();
  const auto& allAssets = pCurator->GetKnownSubAssets();

  ezUInt64 uiAssetHash = 1;

  for (auto it = allAssets->GetIterator(); it.IsValid(); ++it)
  {
    const auto& asset = it.Value();

    if (asset.m_pAssetInfo->GetManager() != this)
      continue;

    uiAssetHash += pCurator->GetAssetDependencyHash(it.Key());
  }

  ezStringBuilder decalFile = ezToolsProject::GetSingleton()->GetProjectDirectory();
  decalFile.AppendPath("AssetCache", GetDecalTexturePath(pAssetProfile));

  if (IsDecalTextureUpToDate(decalFile, uiAssetHash))
    return ezStatus(EZ_SUCCESS);

  ezTextureAtlasCreationDesc atlasDesc;

  // find all decal assets, extract their file information to pass it along to TexConv
  {
    atlasDesc.m_Layers.SetCount(3);
    atlasDesc.m_Layers[0].m_Usage = ezTexConvUsage::Color;
    atlasDesc.m_Layers[1].m_Usage = ezTexConvUsage::NormalMap;
    atlasDesc.m_Layers[2].m_Usage = ezTexConvUsage::Linear;
    atlasDesc.m_Layers[2].m_uiNumChannels = 3;

    atlasDesc.m_Items.Reserve(64);

    ezQtEditorApp* pEditorApp = ezQtEditorApp::GetSingleton();
    ezStringBuilder sAbsPath;

    for (auto it = allAssets->GetIterator(); it.IsValid(); ++it)
    {
      const auto& asset = it.Value();

      if (asset.m_pAssetInfo->GetManager() != this)
        continue;

      EZ_LOG_BLOCK("Decal", asset.m_pAssetInfo->m_sDataDirRelativePath.GetData());

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

        auto pDecalProps = pDecalAsset->GetProperties();
        item.m_uiFlags = 0;
        item.m_uiFlags |= pDecalProps->NeedsNormal() ? DECAL_USE_NORMAL : 0;
        item.m_uiFlags |= pDecalProps->NeedsORM() ? DECAL_USE_ORM : 0;
        item.m_uiFlags |= pDecalProps->NeedsEmissive() ? DECAL_USE_EMISSIVE : 0;
        item.m_uiFlags |= pDecalProps->m_bBlendModeColorize ? DECAL_BLEND_MODE_COLORIZE : 0;

        if (pDecalProps->NeedsBaseColor())
        {
          sAbsPath = pDecalAsset->GetProperties()->m_sBaseColor;
          if (sAbsPath.IsEmpty() || !pEditorApp->MakeDataDirectoryRelativePathAbsolute(sAbsPath))
          {
            return ezStatus(ezFmt("Invalid base color texture path '{0}'", sAbsPath));
          }

          item.m_sLayerInput[0] = sAbsPath;
        }

        if (pDecalProps->NeedsNormal())
        {
          sAbsPath = pDecalAsset->GetProperties()->m_sNormal;
          if (sAbsPath.IsEmpty() || !pEditorApp->MakeDataDirectoryRelativePathAbsolute(sAbsPath))
          {
            return ezStatus(ezFmt("Invalid normal texture path '{0}'", sAbsPath));
          }

          item.m_sLayerInput[1] = sAbsPath;
        }

        if (pDecalProps->NeedsORM())
        {
          sAbsPath = pDecalAsset->GetProperties()->m_sORM;
          if (sAbsPath.IsEmpty() || !pEditorApp->MakeDataDirectoryRelativePathAbsolute(sAbsPath))
          {
            return ezStatus(ezFmt("Invalid ORM texture path '{0}'", sAbsPath));
          }

          item.m_sLayerInput[2] = sAbsPath;
        }

        if (pDecalProps->NeedsEmissive())
        {
          sAbsPath = pDecalAsset->GetProperties()->m_sEmissive;
          if (sAbsPath.IsEmpty() || !pEditorApp->MakeDataDirectoryRelativePathAbsolute(sAbsPath))
          {
            return ezStatus(ezFmt("Invalid emissive texture path '{0}'", sAbsPath));
          }

          item.m_sLayerInput[2] = sAbsPath;
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

    header.SetFileHashAndVersion(uiAssetHash, uiVersion);
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

bool ezDecalAssetDocumentManager::IsDecalTextureUpToDate(const char* szDecalFile, ezUInt64 uiAssetHash) const
{
  ezFileReader file;
  if (file.Open(szDecalFile).Succeeded())
  {
    ezAssetFileHeader header;
    header.Read(file);

    // file still valid
    if (header.GetFileHash() == uiAssetHash)
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
  arguments << "Atlas";

  arguments << "-compression";
  arguments << ToCompressionMode(ezTexConvCompressionMode::High);

  arguments << "-mipmaps";
  arguments << ToMipmapMode(ezTexConvMipmapMode::Linear);

  arguments << "-atlasDesc";
  arguments << QString(szInputFile);

  ezStringBuilder cmd;
  for (ezInt32 i = 0; i < arguments.size(); ++i)
    cmd.Append(" ", arguments[i].toUtf8().data());

  ezLog::Debug("TexConv.exe{0}", cmd);

  EZ_SUCCEED_OR_RETURN(ezQtEditorApp::GetSingleton()->ExecuteTool("TexConv.exe", arguments, 60, ezLog::GetThreadLocalLogSystem()));

  return ezStatus(EZ_SUCCESS);
}
