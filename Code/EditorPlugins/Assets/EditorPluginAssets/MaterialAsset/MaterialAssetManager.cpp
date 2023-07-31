#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/MaterialAsset/MaterialAsset.h>
#include <EditorPluginAssets/MaterialAsset/MaterialAssetManager.h>
#include <EditorPluginAssets/MaterialAsset/MaterialAssetWindow.moc.h>
#include <ToolsFoundation/Assets/AssetFileExtensionWhitelist.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMaterialAssetDocumentManager, 1, ezRTTIDefaultAllocator<ezMaterialAssetDocumentManager>)
  ;
EZ_END_DYNAMIC_REFLECTED_TYPE;

const char* const ezMaterialAssetDocumentManager::s_szShaderOutputTag = "VISUAL_SHADER";

ezMaterialAssetDocumentManager::ezMaterialAssetDocumentManager()
{
  ezDocumentManager::s_Events.AddEventHandler(ezMakeDelegate(&ezMaterialAssetDocumentManager::OnDocumentManagerEvent, this));

  // additional whitelist for non-asset files where an asset may be selected
  ezAssetFileExtensionWhitelist::AddAssetFileExtension("CompatibleAsset_Material", "ezMaterial");

  m_DocTypeDesc.m_sDocumentTypeName = "Material";
  m_DocTypeDesc.m_sFileExtension = "ezMaterialAsset";
  m_DocTypeDesc.m_sIcon = ":/AssetIcons/Material.svg";
  m_DocTypeDesc.m_IconColorGroup = ezColorScheme::Rendering;
  m_DocTypeDesc.m_pDocumentType = ezGetStaticRTTI<ezMaterialAssetDocument>();
  m_DocTypeDesc.m_pManager = this;
  m_DocTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_Material");

  m_DocTypeDesc.m_sResourceFileExtension = "ezMaterialBin";
  m_DocTypeDesc.m_AssetDocumentFlags = ezAssetDocumentFlags::SupportsThumbnail;
}

ezMaterialAssetDocumentManager::~ezMaterialAssetDocumentManager()
{
  ezDocumentManager::s_Events.RemoveEventHandler(ezMakeDelegate(&ezMaterialAssetDocumentManager::OnDocumentManagerEvent, this));
}

ezString ezMaterialAssetDocumentManager::GetRelativeOutputFileName(const ezAssetDocumentTypeDescriptor* pTypeDescriptor, const char* szDataDirectory, const char* szDocumentPath, const char* szOutputTag, const ezPlatformProfile* pAssetProfile) const
{
  if (ezStringUtils::IsEqual(szOutputTag, s_szShaderOutputTag))
  {
    ezStringBuilder sRelativePath(szDocumentPath);
    sRelativePath.MakeRelativeTo(szDataDirectory).IgnoreResult();
    ezAssetDocumentManager::GenerateOutputFilename(sRelativePath, pAssetProfile, "autogen.ezShader", false);
    return sRelativePath;
  }

  return SUPER::GetRelativeOutputFileName(pTypeDescriptor, szDataDirectory, szDocumentPath, szOutputTag, pAssetProfile);
}


bool ezMaterialAssetDocumentManager::IsOutputUpToDate(const char* szDocumentPath, const char* szOutputTag, ezUInt64 uiHash, const ezAssetDocumentTypeDescriptor* pTypeDescriptor)
{
  if (ezStringUtils::IsEqual(szOutputTag, s_szShaderOutputTag))
  {
    const ezString sTargetFile = GetAbsoluteOutputFileName(pTypeDescriptor, szDocumentPath, szOutputTag);

    ezStringBuilder sExpectedHeader;
    sExpectedHeader.Format("//{0}|{1}\n", uiHash, pTypeDescriptor->m_pDocumentType->GetTypeVersion());

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

  return ezAssetDocumentManager::IsOutputUpToDate(szDocumentPath, szOutputTag, uiHash, pTypeDescriptor);
}


void ezMaterialAssetDocumentManager::OnDocumentManagerEvent(const ezDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case ezDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == ezGetStaticRTTI<ezMaterialAssetDocument>())
      {
        new ezQtMaterialAssetDocumentWindow(static_cast<ezMaterialAssetDocument*>(e.m_pDocument)); // NOLINT: not a memory leak
      }
    }
    break;

    default:
      break;
  }
}

void ezMaterialAssetDocumentManager::InternalCreateDocument(const char* szDocumentTypeName, const char* szPath, bool bCreateNewDocument, ezDocument*& out_pDocument, const ezDocumentObject* pOpenContext)
{
  out_pDocument = new ezMaterialAssetDocument(szPath);
}

void ezMaterialAssetDocumentManager::InternalGetSupportedDocumentTypes(ezDynamicArray<const ezDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
}
