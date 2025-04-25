#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorPluginAssets/DecalAsset/DecalAsset.h>
#include <EditorPluginAssets/DecalAsset/DecalAssetManager.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_ENUM(ezDecalMode, 1)
  EZ_ENUM_CONSTANT(ezDecalMode::BaseColor),
  EZ_ENUM_CONSTANT(ezDecalMode::BaseColorNormal),
  EZ_ENUM_CONSTANT(ezDecalMode::BaseColorORM),
  EZ_ENUM_CONSTANT(ezDecalMode::BaseColorNormalORM),
  EZ_ENUM_CONSTANT(ezDecalMode::BaseColorEmissive)
EZ_END_STATIC_REFLECTED_ENUM;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezDecalAssetProperties, 3, ezRTTIDefaultAllocator<ezDecalAssetProperties>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ENUM_MEMBER_PROPERTY("Mode", ezDecalMode, m_Mode),
    EZ_MEMBER_PROPERTY("BlendModeColorize", m_bBlendModeColorize),
    EZ_MEMBER_PROPERTY("AlphaMask", m_sAlphaMask)->AddAttributes(new ezFileBrowserAttribute("Select Alpha Mask", ezFileBrowserAttribute::ImagesLdrOnly)),
    EZ_MEMBER_PROPERTY("BaseColor", m_sBaseColor)->AddAttributes(new ezFileBrowserAttribute("Select Base Color Map", ezFileBrowserAttribute::ImagesLdrOnly)),
    EZ_MEMBER_PROPERTY("Normal", m_sNormal)->AddAttributes(new ezFileBrowserAttribute("Select Normal Map", ezFileBrowserAttribute::ImagesLdrOnly), new ezDefaultValueAttribute(ezStringView("Textures/NeutralNormal.tga"))), // wrap in ezStringView to prevent a memory leak report
    EZ_MEMBER_PROPERTY("ORM", m_sORM)->AddAttributes(new ezFileBrowserAttribute("Select ORM Map", ezFileBrowserAttribute::ImagesLdrOnly)),
    EZ_MEMBER_PROPERTY("Emissive", m_sEmissive)->AddAttributes(new ezFileBrowserAttribute("Select Emissive Map", ezFileBrowserAttribute::ImagesLdrOnly)),
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
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezDecalAssetDocument, 5, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezDecalAssetDocument::ezDecalAssetDocument(ezStringView sDocumentPath)
  : ezSimpleAssetDocument<ezDecalAssetProperties>(sDocumentPath, ezAssetDocEngineConnection::Simple, true)
{
}

ezTransformStatus ezDecalAssetDocument::InternalTransformAsset(ezStreamWriter& stream, ezStringView sOutputTag, const ezPlatformProfile* pAssetProfile, const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags)
{
  return static_cast<ezDecalAssetDocumentManager*>(GetAssetDocumentManager())->GenerateDecalTexture(pAssetProfile);
}

ezTransformStatus ezDecalAssetDocument::InternalCreateThumbnail(const ThumbnailInfo& Unused)
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
    EZ_SUCCEED_OR_RETURN(ezOSFile::CreateDirectoryStructure(sDir));

    arguments << "-thumbnailOut";
    arguments << QString::fromUtf8(sThumbnail.GetData());

    arguments << "-thumbnailRes";
    arguments << "256";
  }

  {
    ezQtEditorApp* pEditorApp = ezQtEditorApp::GetSingleton();

    temp.SetFormat("-in0");

    ezStringBuilder sAbsPath = pProp->m_sBaseColor;
    if (!pEditorApp->MakeDataDirectoryRelativePathAbsolute(sAbsPath))
    {
      return ezStatus(ezFmt("Failed to make path absolute: '{}'", sAbsPath));
    }

    arguments << temp.GetData();
    arguments << QString(sAbsPath.GetData());

    if (!pProp->m_sAlphaMask.IsEmpty())
    {
      ezStringBuilder sAbsPath2 = pProp->m_sAlphaMask;
      if (!pEditorApp->MakeDataDirectoryRelativePathAbsolute(sAbsPath2))
      {
        return ezStatus(ezFmt("Failed to make path absolute: '{}'", sAbsPath2));
      }

      arguments << "-in1";
      arguments << QString(sAbsPath2.GetData());

      arguments << "-rgb";
      arguments << "in0.rgb";

      arguments << "-a";
      arguments << "in1.r";
    }
    else
    {
      arguments << "-rgba";
      arguments << "in0.rgba";
    }
  }

  EZ_SUCCEED_OR_RETURN(ezQtEditorApp::GetSingleton()->ExecuteTool("ezTexConv", arguments, 180, ezLog::GetThreadLocalLogSystem()));

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

ezDecalAssetDocumentGenerator::~ezDecalAssetDocumentGenerator() = default;

void ezDecalAssetDocumentGenerator::GetImportModes(ezStringView sAbsInputFile, ezDynamicArray<ezAssetDocumentGenerator::ImportMode>& out_modes) const
{
  const ezStringBuilder baseFilename = sAbsInputFile.GetFileName();

  const bool isDecal = (baseFilename.FindSubString_NoCase("decal") != nullptr);

  {
    ezAssetDocumentGenerator::ImportMode& info = out_modes.ExpandAndGetRef();
    info.m_Priority = isDecal ? ezAssetDocGeneratorPriority::HighPriority : ezAssetDocGeneratorPriority::LowPriority;
    info.m_sName = "DecalImport.All";
    info.m_sIcon = ":/AssetIcons/Decal.svg";
  }
}

ezStatus ezDecalAssetDocumentGenerator::Generate(ezStringView sInputFileAbs, ezStringView sMode, ezDynamicArray<ezDocument*>& out_generatedDocuments)
{
  ezStringBuilder sOutFile = sInputFileAbs;
  sOutFile.ChangeFileExtension(GetDocumentExtension());

  if (ezOSFile::ExistsFile(sOutFile))
  {
    ezLog::Info("Skipping decal import, file has been imported before: '{}'", sOutFile);
    return ezStatus(EZ_SUCCESS);
  }

  auto pApp = ezQtEditorApp::GetSingleton();

  ezStringBuilder sInputFileRel = sInputFileAbs;
  pApp->MakePathDataDirectoryRelative(sInputFileRel);

  ezDocument* pDoc = pApp->CreateDocument(sOutFile, ezDocumentFlags::None);
  if (pDoc == nullptr)
    return ezStatus("Could not create target document");

  out_generatedDocuments.PushBack(pDoc);

  ezDecalAssetDocument* pAssetDoc = ezDynamicCast<ezDecalAssetDocument*>(pDoc);

  auto& accessor = pAssetDoc->GetPropertyObject()->GetTypeAccessor();
  accessor.SetValue("BaseColor", sInputFileRel.GetView());

  ezLog::Success("Imported decal: '{}'", sOutFile);

  return ezStatus(EZ_SUCCESS);
}
