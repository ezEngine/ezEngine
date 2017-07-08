#include <PCH.h>
#include <EditorPluginAssets/DecalAsset/DecalAsset.h>
#include <EditorPluginAssets/DecalAsset/DecalAssetManager.h>
#include <Core/Assets/AssetFileHeader.h>
#include <Foundation/IO/OSFile.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Assets/AssetCurator.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezDecalAssetProperties, 1, ezRTTIDefaultAllocator<ezDecalAssetProperties>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("BaseColor", m_sBaseColor)->AddAttributes(new ezFileBrowserAttribute("Select Base Color Map", "*.dds;*.tga;*.png;*.jpg;*.jpeg")),
    EZ_MEMBER_PROPERTY("Normal", m_sNormal)->AddAttributes(new ezFileBrowserAttribute("Select Normal Map", "*.dds;*.tga;*.png;*.jpg;*.jpeg")),

  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE


EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezDecalAssetDocument, 2, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE

ezDecalAssetDocument::ezDecalAssetDocument(const char* szDocumentPath)
  : ezSimpleAssetDocument<ezDecalAssetProperties>(szDocumentPath, false)
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

  ezLog::Debug("TexConv.exe{0}", cmd.GetData());

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
