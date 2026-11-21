#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/Texture3DAsset/Texture3DAsset.h>
#include <EditorPluginAssets/Texture3DAsset/Texture3DAssetManager.h>
#include <EditorPluginAssets/Texture3DAsset/Texture3DAssetWindow.moc.h>
#include <EditorPluginAssets/TextureAsset/TextureAssetManager.h>
#include <ToolsFoundation/Assets/AssetFileExtensionWhitelist.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTexture3DAssetDocumentManager, 1, ezRTTIDefaultAllocator<ezTexture3DAssetDocumentManager>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezTexture3DAssetDocumentManager::ezTexture3DAssetDocumentManager()
{
  ezDocumentManager::s_Events.AddEventHandler(ezMakeDelegate(&ezTexture3DAssetDocumentManager::OnDocumentManagerEvent, this));

  // additional whitelist for non-asset files where an asset may be selected
  ezAssetFileExtensionWhitelist::AddAssetFileExtension("CompatibleAsset_Texture_3D", "dds");

  // texture asset source files
  ezAssetFileExtensionWhitelist::AddAssetFileExtension("Image3D", "dds");

  m_DocTypeDesc.m_sDocumentTypeName = "Texture 3D";
  m_DocTypeDesc.m_sFileExtension = "ezTexture3DAsset";
  m_DocTypeDesc.m_sIcon = ":/AssetIcons/Texture_Cube.svg";
  m_DocTypeDesc.m_sAssetCategory = "Rendering";
  m_DocTypeDesc.m_pDocumentType = ezGetStaticRTTI<ezTexture3DAssetDocument>();
  m_DocTypeDesc.m_pManager = this;
  m_DocTypeDesc.m_sResourceFileExtension = "ezBinTexture3D";
  m_DocTypeDesc.m_AssetDocumentFlags = ezAssetDocumentFlags::AutoThumbnailOnTransform;
  m_DocTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_Texture_3D");
}

ezTexture3DAssetDocumentManager::~ezTexture3DAssetDocumentManager()
{
  ezDocumentManager::s_Events.RemoveEventHandler(ezMakeDelegate(&ezTexture3DAssetDocumentManager::OnDocumentManagerEvent, this));
}

ezUInt64 ezTexture3DAssetDocumentManager::ComputeAssetProfileHashImpl(const ezPlatformProfile* pAssetProfile) const
{
  return pAssetProfile->GetTypeConfig<ezTextureAssetProfileConfig>()->m_uiMaxResolution;
}

void ezTexture3DAssetDocumentManager::OnDocumentManagerEvent(const ezDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case ezDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == ezGetStaticRTTI<ezTexture3DAssetDocument>())
      {
        new ezQtTexture3DAssetDocumentWindow(static_cast<ezTexture3DAssetDocument*>(e.m_pDocument)); // NOLINT: Not a memory leak
      }
    }
    break;

    default:
      break;
  }
}

void ezTexture3DAssetDocumentManager::InternalCreateDocument(
  ezStringView sDocumentTypeName, ezStringView sPath, bool bCreateNewDocument, ezDocument*& out_pDocument, const ezDocumentObject* pOpenContext)
{
  ezTexture3DAssetDocument* pDoc = new ezTexture3DAssetDocument(sPath);
  out_pDocument = pDoc;
}

void ezTexture3DAssetDocumentManager::InternalGetSupportedDocumentTypes(ezDynamicArray<const ezDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
}
