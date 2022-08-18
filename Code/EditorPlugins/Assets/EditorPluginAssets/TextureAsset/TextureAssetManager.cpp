#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/TextureAsset/TextureAsset.h>
#include <EditorPluginAssets/TextureAsset/TextureAssetManager.h>
#include <EditorPluginAssets/TextureAsset/TextureAssetWindow.moc.h>
#include <ToolsFoundation/Assets/AssetFileExtensionWhitelist.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTextureAssetProfileConfig, 1, ezRTTIDefaultAllocator<ezTextureAssetProfileConfig>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("MaxResolution", m_uiMaxResolution)->AddAttributes(new ezDefaultValueAttribute(16 * 1024)),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTextureAssetDocumentManager, 1, ezRTTIDefaultAllocator<ezTextureAssetDocumentManager>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezTextureAssetDocumentManager::ezTextureAssetDocumentManager()
{
  ezDocumentManager::s_Events.AddEventHandler(ezMakeDelegate(&ezTextureAssetDocumentManager::OnDocumentManagerEvent, this));

  // additional whitelist for non-asset files where an asset may be selected
  ezAssetFileExtensionWhitelist::AddAssetFileExtension("CompatibleAsset_Texture_2D", "dds");
  ezAssetFileExtensionWhitelist::AddAssetFileExtension("CompatibleAsset_Texture_2D", "color");

  // texture asset source files
  ezAssetFileExtensionWhitelist::AddAssetFileExtension("Image2D", "dds");
  ezAssetFileExtensionWhitelist::AddAssetFileExtension("Image2D", "tga");

  m_DocTypeDesc.m_sDocumentTypeName = "Texture 2D";
  m_DocTypeDesc.m_sFileExtension = "ezTextureAsset";
  m_DocTypeDesc.m_sIcon = ":/AssetIcons/Texture_2D.png";
  m_DocTypeDesc.m_pDocumentType = ezGetStaticRTTI<ezTextureAssetDocument>();
  m_DocTypeDesc.m_pManager = this;
  m_DocTypeDesc.m_sResourceFileExtension = "ezTexture2D";
  m_DocTypeDesc.m_AssetDocumentFlags = ezAssetDocumentFlags::AutoThumbnailOnTransform;
  m_DocTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_Texture_2D");

  m_DocTypeDesc2.m_sDocumentTypeName = "Render Target";
  m_DocTypeDesc2.m_sFileExtension = "ezRenderTargetAsset";
  m_DocTypeDesc2.m_sIcon = ":/AssetIcons/Render_Target.png";
  m_DocTypeDesc2.m_pDocumentType = ezGetStaticRTTI<ezTextureAssetDocument>();
  m_DocTypeDesc2.m_pManager = this;
  m_DocTypeDesc2.m_sResourceFileExtension = "ezRenderTarget";
  m_DocTypeDesc2.m_AssetDocumentFlags = ezAssetDocumentFlags::AutoTransformOnSave;
  m_DocTypeDesc2.m_CompatibleTypes.PushBack("CompatibleAsset_Texture_2D"); // render targets can also be used as 2D textures
  m_DocTypeDesc2.m_CompatibleTypes.PushBack("CompatibleAsset_Texture_Target");

  ezQtImageCache::GetSingleton()->RegisterTypeImage("Render Target", QPixmap(":/AssetIcons/Render_Target.png"));
}

ezTextureAssetDocumentManager::~ezTextureAssetDocumentManager()
{
  ezDocumentManager::s_Events.RemoveEventHandler(ezMakeDelegate(&ezTextureAssetDocumentManager::OnDocumentManagerEvent, this));
}

ezUInt64 ezTextureAssetDocumentManager::ComputeAssetProfileHashImpl(const ezPlatformProfile* pAssetProfile) const
{
  return pAssetProfile->GetTypeConfig<ezTextureAssetProfileConfig>()->m_uiMaxResolution;
}

void ezTextureAssetDocumentManager::OnDocumentManagerEvent(const ezDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case ezDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == ezGetStaticRTTI<ezTextureAssetDocument>())
      {
        ezQtTextureAssetDocumentWindow* pDocWnd = new ezQtTextureAssetDocumentWindow(static_cast<ezTextureAssetDocument*>(e.m_pDocument));
      }
    }
    break;

    default:
      break;
  }
}

void ezTextureAssetDocumentManager::InternalCreateDocument(
  const char* szDocumentTypeName, const char* szPath, bool bCreateNewDocument, ezDocument*& out_pDocument, const ezDocumentObject* pOpenContext)
{
  ezTextureAssetDocument* pDoc = new ezTextureAssetDocument(szPath);
  out_pDocument = pDoc;

  if (ezStringUtils::IsEqual(szDocumentTypeName, "Render Target"))
  {
    pDoc->m_bIsRenderTarget = true;
  }
}

void ezTextureAssetDocumentManager::InternalGetSupportedDocumentTypes(ezDynamicArray<const ezDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
  inout_DocumentTypes.PushBack(&m_DocTypeDesc2);
}
