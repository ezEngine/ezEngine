#include <PCH.h>

#include <EditorPluginAssets/MaterialAsset/MaterialAsset.h>
#include <EditorPluginAssets/MaterialAsset/MaterialAssetManager.h>
#include <EditorPluginAssets/MaterialAsset/MaterialAssetWindow.moc.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <ToolsFoundation/Assets/AssetFileExtensionWhitelist.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMaterialAssetDocumentManager, 1, ezRTTIDefaultAllocator<ezMaterialAssetDocumentManager>);
EZ_END_DYNAMIC_REFLECTED_TYPE;

const char* const ezMaterialAssetDocumentManager::s_szShaderOutputTag = "VISUAL_SHADER";

ezMaterialAssetDocumentManager::ezMaterialAssetDocumentManager()
{
  ezDocumentManager::s_Events.AddEventHandler(ezMakeDelegate(&ezMaterialAssetDocumentManager::OnDocumentManagerEvent, this));

  // additional whitelist for non-asset files where an asset may be selected
  ezAssetFileExtensionWhitelist::AddAssetFileExtension("Material", "ezMaterial");

  m_AssetDesc.m_bCanCreate = true;
  m_AssetDesc.m_sDocumentTypeName = "Material Asset";
  m_AssetDesc.m_sFileExtension = "ezMaterialAsset";
  m_AssetDesc.m_sIcon = ":/AssetIcons/Material.png";
  m_AssetDesc.m_pDocumentType = ezGetStaticRTTI<ezMaterialAssetDocument>();
  m_AssetDesc.m_pManager = this;
}

ezMaterialAssetDocumentManager::~ezMaterialAssetDocumentManager()
{
  ezDocumentManager::s_Events.RemoveEventHandler(ezMakeDelegate(&ezMaterialAssetDocumentManager::OnDocumentManagerEvent, this));
}


ezBitflags<ezAssetDocumentFlags>
ezMaterialAssetDocumentManager::GetAssetDocumentTypeFlags(const ezDocumentTypeDescriptor* pDescriptor) const
{
  EZ_ASSERT_DEBUG(pDescriptor->m_pManager == this, "Given type descriptor is not part of this document manager!");
  return ezAssetDocumentFlags::SupportsThumbnail;
}


ezString ezMaterialAssetDocumentManager::GetRelativeOutputFileName(const char* szDataDirectory, const char* szDocumentPath,
                                                                   const char* szOutputTag, const ezAssetPlatformConfig* pPlatformConfig) const
{
  if (ezStringUtils::IsEqual(szOutputTag, s_szShaderOutputTag))
  {
    ezStringBuilder sRelativePath(szDocumentPath);
    sRelativePath.MakeRelativeTo(szDataDirectory);
    ezAssetDocumentManager::GenerateOutputFilename(sRelativePath, pPlatformConfig, "autogen.ezShader", false);
    return sRelativePath;
  }

  return SUPER::GetRelativeOutputFileName(szDataDirectory, szDocumentPath, szOutputTag, pPlatformConfig);
}


bool ezMaterialAssetDocumentManager::IsOutputUpToDate(const char* szDocumentPath, const char* szOutputTag, ezUInt64 uiHash,
                                                      ezUInt16 uiTypeVersion)
{
  if (ezStringUtils::IsEqual(szOutputTag, s_szShaderOutputTag))
  {
    const ezString sTargetFile = GetAbsoluteOutputFileName(szDocumentPath, szOutputTag);

    ezStringBuilder sExpectedHeader;
    sExpectedHeader.Format("//{0}|{1}\n", uiHash, uiTypeVersion);

    ezFileReader file;
    if (file.Open(sTargetFile, 256).Failed())
      return false;

    // this might happen if writing to the file failed
    if (file.GetFileSize() < sExpectedHeader.GetElementCount())
      return false;

    ezUInt8 Temp[256] = {0};
    const ezUInt32 uiRead = (ezUInt32)file.ReadBytes(Temp, sExpectedHeader.GetElementCount());
    ezStringBuilder sFileHeader = ezStringView((const char*)&Temp[0], (const char*)&Temp[uiRead]);

    return sFileHeader.IsEqual(sExpectedHeader);
  }

  return ezAssetDocumentManager::IsOutputUpToDate(szDocumentPath, szOutputTag, uiHash, uiTypeVersion);
}


void ezMaterialAssetDocumentManager::OnDocumentManagerEvent(const ezDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case ezDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == ezGetStaticRTTI<ezMaterialAssetDocument>())
      {
        ezQtMaterialAssetDocumentWindow* pDocWnd =
            new ezQtMaterialAssetDocumentWindow(static_cast<ezMaterialAssetDocument*>(e.m_pDocument));
      }
    }
    break;
  }
}

ezStatus ezMaterialAssetDocumentManager::InternalCreateDocument(const char* szDocumentTypeName, const char* szPath, bool bCreateNewDocument,
                                                                ezDocument*& out_pDocument)
{
  out_pDocument = new ezMaterialAssetDocument(szPath);

  return ezStatus(EZ_SUCCESS);
}

void ezMaterialAssetDocumentManager::InternalGetSupportedDocumentTypes(
    ezDynamicArray<const ezDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_AssetDesc);
}
