#include <PCH.h>
#include <EditorPluginAssets/DecalAsset/DecalAsset.h>
#include <EditorPluginAssets/DecalAsset/DecalAssetManager.h>
#include <Core/Assets/AssetFileHeader.h>
#include <Foundation/IO/OSFile.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezDecalMode, 1)
  EZ_ENUM_CONSTANT(ezDecalMode::All),
  EZ_ENUM_CONSTANT(ezDecalMode::BaseColor),
  EZ_ENUM_CONSTANT(ezDecalMode::BaseColorRoughness),
  EZ_ENUM_CONSTANT(ezDecalMode::NormalRoughnessOcclusion),
  EZ_ENUM_CONSTANT(ezDecalMode::Emissive)
EZ_END_STATIC_REFLECTED_ENUM;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezDecalAssetProperties, 1, ezRTTIDefaultAllocator<ezDecalAssetProperties>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ENUM_MEMBER_PROPERTY("Mode", ezDecalMode, m_Mode),
    EZ_MEMBER_PROPERTY("BaseColor", m_sBaseColor)->AddAttributes(new ezFileBrowserAttribute("Select Base Color Map", "*.dds;*.tga;*.png;*.jpg;*.jpeg")),
    EZ_MEMBER_PROPERTY("Normal", m_sNormal)->AddAttributes(new ezFileBrowserAttribute("Select Normal Map", "*.dds;*.tga;*.png;*.jpg;*.jpeg")/*, new ezDefaultValueAttribute(ezUntrackedString("Textures/NeutralNormal.tga"))*/), // would report a memory leak
    EZ_MEMBER_PROPERTY("Roughness", m_sRoughness)->AddAttributes(new ezFileBrowserAttribute("Select Roughness Map", "*.dds;*.tga;*.png;*.jpg;*.jpeg")),
    EZ_MEMBER_PROPERTY("RoughnessValue", m_fRoughnessValue)->AddAttributes(new ezDefaultValueAttribute(0.7f), new ezClampValueAttribute(0.0f, 1.0f)),
    EZ_MEMBER_PROPERTY("Metallic", m_sMetallic)->AddAttributes(new ezFileBrowserAttribute("Select Metallic Map", "*.dds;*.tga;*.png;*.jpg;*.jpeg")),
    EZ_MEMBER_PROPERTY("MetallicValue", m_fMetallicValue)->AddAttributes(new ezDefaultValueAttribute(0.0f), new ezClampValueAttribute(0.0f, 1.0f)),
    EZ_MEMBER_PROPERTY("Emissive", m_sEmissive)->AddAttributes(new ezFileBrowserAttribute("Select Emissive Map", "*.dds;*.tga;*.png;*.jpg;*.jpeg")),
    EZ_MEMBER_PROPERTY("Occlusion", m_sOcclusion)->AddAttributes(new ezFileBrowserAttribute("Select Occlusion Map", "*.dds;*.tga;*.png;*.jpg;*.jpeg")),
    EZ_MEMBER_PROPERTY("OcclusionValue", m_fOcclusionValue)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.0f, 1.0f)),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezDecalAssetProperties::ezDecalAssetProperties()
  : m_fRoughnessValue(0.7f)
  , m_fMetallicValue(0.0f)
  , m_fOcclusionValue(1.0f)
{

}

void ezDecalAssetProperties::PropertyMetaStateEventHandler(ezPropertyMetaStateEvent& e)
{
  if (e.m_pObject->GetTypeAccessor().GetType() == ezGetStaticRTTI<ezDecalAssetProperties>())
  {
    ezInt64 mode = e.m_pObject->GetTypeAccessor().GetValue("Mode").ConvertTo<ezInt64>();

    auto& props = *e.m_pPropertyStates;

    props["Emissive"].m_Visibility = ezPropertyUiState::Invisible;

    if (mode != ezDecalMode::All)
    {
      props["BaseColor"].m_Visibility = ezPropertyUiState::Invisible;
      props["Normal"].m_Visibility = ezPropertyUiState::Invisible;
      props["Roughness"].m_Visibility = ezPropertyUiState::Invisible;
      props["RoughnessValue"].m_Visibility = ezPropertyUiState::Invisible;
      props["Metallic"].m_Visibility = ezPropertyUiState::Invisible;
      props["MetallicValue"].m_Visibility = ezPropertyUiState::Invisible;
      props["Occlusion"].m_Visibility = ezPropertyUiState::Invisible;
      props["OcclusionValue"].m_Visibility = ezPropertyUiState::Invisible;

      if (mode == ezDecalMode::BaseColor)
      {
        props["BaseColor"].m_Visibility = ezPropertyUiState::Default;
      }
      else if (mode == ezDecalMode::BaseColorRoughness)
      {
        props["BaseColor"].m_Visibility = ezPropertyUiState::Default;
        props["Roughness"].m_Visibility = ezPropertyUiState::Default;
        props["RoughnessValue"].m_Visibility = ezPropertyUiState::Default;
      }
      else if (mode == ezDecalMode::NormalRoughnessOcclusion)
      {
        props["Normal"].m_Visibility = ezPropertyUiState::Default;
        props["Roughness"].m_Visibility = ezPropertyUiState::Default;
        props["RoughnessValue"].m_Visibility = ezPropertyUiState::Default;
        props["Occlusion"].m_Visibility = ezPropertyUiState::Default;
        props["OcclusionValue"].m_Visibility = ezPropertyUiState::Default;
      }
      else if (mode == ezDecalMode::Emissive)
      {
        props["Emissive"].m_Visibility = ezPropertyUiState::Default;
      }
    }
  }
}


EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezDecalAssetDocument, 3, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezDecalAssetDocument::ezDecalAssetDocument(const char* szDocumentPath)
  : ezSimpleAssetDocument<ezDecalAssetProperties>(szDocumentPath, true)
{
}

const char* ezDecalAssetDocument::QueryAssetType() const
{
  return "Decal";
}

ezStatus ezDecalAssetDocument::InternalTransformAsset(ezStreamWriter& stream, const char* szOutputTag, const char* szPlatform, const ezAssetFileHeader& AssetHeader, bool bTriggeredManually)
{
  return static_cast<ezDecalAssetDocumentManager*>(GetAssetDocumentManager())->GenerateDecalTexture(szPlatform);
}

ezStatus ezDecalAssetDocument::InternalCreateThumbnail(const ezAssetFileHeader& AssetHeader)
{
  const ezDecalAssetProperties* pProp = GetProperties();

  QStringList arguments;
  ezStringBuilder temp;

  arguments << "-premulalpha";
  arguments << "-srgb";

  const ezStringBuilder sThumbnail = GetThumbnailFilePath();
  {
    // Thumbnail
    const ezStringBuilder sDir = sThumbnail.GetFileDirectory();
    ezOSFile::CreateDirectoryStructure(sDir);

    arguments << "-thumbnail";
    arguments << QString::fromUtf8(sThumbnail.GetData());
  }

  {
    ezQtEditorApp* pEditorApp = ezQtEditorApp::GetSingleton();

    temp.Format("-in0");

    ezStringBuilder sAbsPath = pProp->m_sBaseColor;
    if (!pEditorApp->MakeDataDirectoryRelativePathAbsolute(sAbsPath))
    {
      return ezStatus("Failed to make path absolute");
    }

    arguments << temp.GetData();
    arguments << QString(sAbsPath.GetData());
  }

  ezStringBuilder cmd;
  for (ezInt32 i = 0; i < arguments.size(); ++i)
    cmd.Append(" ", arguments[i].toUtf8().data());

  ezLog::Debug("TexConv.exe{0}", cmd);

  EZ_SUCCEED_OR_RETURN(ezQtEditorApp::GetSingleton()->ExecuteTool("TexConv.exe", arguments, 60, ezLog::GetThreadLocalLogSystem()));

  {
    ezUInt64 uiThumbnailHash = ezAssetCurator::GetSingleton()->GetAssetReferenceHash(GetGuid());
    EZ_ASSERT_DEV(uiThumbnailHash != 0, "Thumbnail hash should never be zero when reaching this point!");
    ezAssetFileHeader assetThumbnailHeader;
    assetThumbnailHeader.SetFileHashAndVersion(uiThumbnailHash, GetAssetTypeVersion());
    AppendThumbnailInfo(sThumbnail, assetThumbnailHeader);
    InvalidateAssetThumbnail();
  }

  return ezStatus(EZ_SUCCESS);
}


//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezDecalAssetDocumentGenerator, 1, ezRTTIDefaultAllocator<ezDecalAssetDocumentGenerator>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezDecalAssetDocumentGenerator::ezDecalAssetDocumentGenerator()
{
  AddSupportedFileType("tga");
  AddSupportedFileType("dds");
  AddSupportedFileType("jpg");
  AddSupportedFileType("jpeg");
  AddSupportedFileType("png");
}

ezDecalAssetDocumentGenerator::~ezDecalAssetDocumentGenerator()
{
}

void ezDecalAssetDocumentGenerator::GetImportModes(const char* szParentDirRelativePath, ezHybridArray<ezAssetDocumentGenerator::Info, 4>& out_Modes) const
{
  ezStringBuilder baseOutputFile = szParentDirRelativePath;

  const ezStringBuilder baseFilename = baseOutputFile.GetFileName();

  baseOutputFile.ChangeFileExtension(GetDocumentExtension());

  /// \todo Make this configurable
  const bool isDecal = (baseFilename.FindSubString_NoCase("decal") != nullptr);

  {
    ezAssetDocumentGenerator::Info& info = out_Modes.ExpandAndGetRef();
    info.m_Priority = isDecal ? ezAssetDocGeneratorPriority::HighPriority : ezAssetDocGeneratorPriority::LowPriority;
    info.m_sName = "DecalImport.All";
    info.m_sOutputFileParentRelative = baseOutputFile;
    info.m_sIcon = ":/AssetIcons/Decal.png";
  }
}

ezStatus ezDecalAssetDocumentGenerator::Generate(const char* szDataDirRelativePath, const ezAssetDocumentGenerator::Info& info, ezDocument*& out_pGeneratedDocument)
{
  auto pApp = ezQtEditorApp::GetSingleton();

  out_pGeneratedDocument = pApp->CreateOrOpenDocument(true, info.m_sOutputFileAbsolute, false, false);
  if (out_pGeneratedDocument == nullptr)
    return ezStatus("Could not create target document");

  ezDecalAssetDocument* pAssetDoc = ezDynamicCast<ezDecalAssetDocument*>(out_pGeneratedDocument);
  if (pAssetDoc == nullptr)
    return ezStatus("Target document is not a valid ezDecalAssetDocument");

  auto& accessor = pAssetDoc->GetPropertyObject()->GetTypeAccessor();
  accessor.SetValue("BaseColor", szDataDirRelativePath);
  accessor.SetValue("Normal", "Textures/NeutralNormal.tga");

  return ezStatus(EZ_SUCCESS);
}




