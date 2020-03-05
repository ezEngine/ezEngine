#include <EditorPluginAssetsPCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorPluginAssets/DecalAsset/DecalAsset.h>
#include <EditorPluginAssets/DecalAsset/DecalAssetManager.h>
#include <Foundation/IO/OSFile.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_ENUM(ezDecalMode, 1)
  EZ_ENUM_CONSTANT(ezDecalMode::BaseColor),
  EZ_ENUM_CONSTANT(ezDecalMode::BaseColorNormal),
  EZ_ENUM_CONSTANT(ezDecalMode::BaseColorORM),
  EZ_ENUM_CONSTANT(ezDecalMode::BaseColorNormalORM),
  EZ_ENUM_CONSTANT(ezDecalMode::BaseColorEmissive)
EZ_END_STATIC_REFLECTED_ENUM;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezDecalAssetProperties, 2, ezRTTIDefaultAllocator<ezDecalAssetProperties>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ENUM_MEMBER_PROPERTY("Mode", ezDecalMode, m_Mode),
    EZ_MEMBER_PROPERTY("BlendModeColorize", m_bBlendModeColorize),
    EZ_MEMBER_PROPERTY("BaseColor", m_sBaseColor)->AddAttributes(new ezFileBrowserAttribute("Select Base Color Map", "*.dds;*.tga;*.png;*.jpg;*.jpeg")),
    EZ_MEMBER_PROPERTY("Normal", m_sNormal)->AddAttributes(new ezFileBrowserAttribute("Select Normal Map", "*.dds;*.tga;*.png;*.jpg;*.jpeg"), new ezDefaultValueAttribute(ezStringView("Textures/NeutralNormal.tga"))), // wrap in ezStringView to prevent a memory leak report
    EZ_MEMBER_PROPERTY("ORM", m_sORM)->AddAttributes(new ezFileBrowserAttribute("Select ORM Map", "*.dds;*.tga;*.png;*.jpg;*.jpeg")),
    EZ_MEMBER_PROPERTY("Emissive", m_sEmissive)->AddAttributes(new ezFileBrowserAttribute("Select Emissive Map", "*.dds;*.tga;*.png;*.jpg;*.jpeg")),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezDecalAssetProperties::ezDecalAssetProperties() = default;

void ezDecalAssetProperties::PropertyMetaStateEventHandler(ezPropertyMetaStateEvent& e)
{
  if (e.m_pObject->GetTypeAccessor().GetType() == ezGetStaticRTTI<ezDecalAssetProperties>())
  {
    ezInt64 mode = e.m_pObject->GetTypeAccessor().GetValue("Mode").ConvertTo<ezInt64>();

    auto& props = *e.m_pPropertyStates;

    props["Normal"].m_Visibility = ezPropertyUiState::Invisible;
    props["ORM"].m_Visibility = ezPropertyUiState::Invisible;
    props["Emissive"].m_Visibility = ezPropertyUiState::Invisible;

    if (mode == ezDecalMode::BaseColorNormal)
    {
      props["Normal"].m_Visibility = ezPropertyUiState::Default;
    }
    else if (mode == ezDecalMode::BaseColorORM)
    {
      props["ORM"].m_Visibility = ezPropertyUiState::Default;
    }
    else if (mode == ezDecalMode::BaseColorNormalORM)
    {
      props["Normal"].m_Visibility = ezPropertyUiState::Default;
      props["ORM"].m_Visibility = ezPropertyUiState::Default;
    }
    else if (mode == ezDecalMode::BaseColorEmissive)
    {
      props["Emissive"].m_Visibility = ezPropertyUiState::Default;
    }
  }
}

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezDecalAssetDocument, 3, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezDecalAssetDocument::ezDecalAssetDocument(const char* szDocumentPath)
  : ezSimpleAssetDocument<ezDecalAssetProperties>(szDocumentPath, ezAssetDocEngineConnection::Simple)
{
}

ezStatus ezDecalAssetDocument::InternalTransformAsset(ezStreamWriter& stream, const char* szOutputTag,
  const ezPlatformProfile* pAssetProfile, const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags)
{
  return static_cast<ezDecalAssetDocumentManager*>(GetAssetDocumentManager())->GenerateDecalTexture(pAssetProfile);
}

ezStatus ezDecalAssetDocument::InternalCreateThumbnail(const ThumbnailInfo& Unused)
{
  const ezDecalAssetProperties* pProp = GetProperties();

  QStringList arguments;
  ezStringBuilder temp;

  const ezStringBuilder sThumbnail = GetThumbnailFilePath();

  arguments << "-usage";
  arguments << "Color";

  {
    // Thumbnail
    const ezStringBuilder sDir = sThumbnail.GetFileDirectory();
    ezOSFile::CreateDirectoryStructure(sDir);

    arguments << "-thumbnailOut";
    arguments << QString::fromUtf8(sThumbnail.GetData());

    arguments << "-thumbnailRes";
    arguments << "256";
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

    arguments << "-rgba";
    arguments << "in0.rgba";
  }

  ezStringBuilder cmd;
  for (ezInt32 i = 0; i < arguments.size(); ++i)
    cmd.Append(" ", arguments[i].toUtf8().data());

  ezLog::Debug("TexConv.exe{0}", cmd);

  EZ_SUCCEED_OR_RETURN(ezQtEditorApp::GetSingleton()->ExecuteTool("TexConv.exe", arguments, 60, ezLog::GetThreadLocalLogSystem()));

  {
    ezUInt64 uiThumbnailHash = ezAssetCurator::GetSingleton()->GetAssetReferenceHash(GetGuid());
    EZ_ASSERT_DEV(uiThumbnailHash != 0, "Thumbnail hash should never be zero when reaching this point!");

    ThumbnailInfo thumbnailInfo;
    thumbnailInfo.SetFileHashAndVersion(uiThumbnailHash, GetAssetTypeVersion());
    AppendThumbnailInfo(sThumbnail, thumbnailInfo);
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

ezDecalAssetDocumentGenerator::~ezDecalAssetDocumentGenerator() {}

void ezDecalAssetDocumentGenerator::GetImportModes(
  const char* szParentDirRelativePath, ezHybridArray<ezAssetDocumentGenerator::Info, 4>& out_Modes) const
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

ezStatus ezDecalAssetDocumentGenerator::Generate(
  const char* szDataDirRelativePath, const ezAssetDocumentGenerator::Info& info, ezDocument*& out_pGeneratedDocument)
{
  auto pApp = ezQtEditorApp::GetSingleton();
  out_pGeneratedDocument = pApp->CreateDocument(info.m_sOutputFileAbsolute, ezDocumentFlags::None);

  if (out_pGeneratedDocument == nullptr)
    return ezStatus("Could not create target document");

  ezDecalAssetDocument* pAssetDoc = ezDynamicCast<ezDecalAssetDocument*>(out_pGeneratedDocument);
  if (pAssetDoc == nullptr)
    return ezStatus("Target document is not a valid ezDecalAssetDocument");

  auto& accessor = pAssetDoc->GetPropertyObject()->GetTypeAccessor();
  accessor.SetValue("BaseColor", szDataDirRelativePath);

  return ezStatus(EZ_SUCCESS);
}
