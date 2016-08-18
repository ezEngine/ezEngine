#include <PCH.h>
#include <EditorPluginAssets/ColorGradientAsset/ColorGradientAssetManager.h>
#include <EditorPluginAssets/ColorGradientAsset/ColorGradientAsset.h>
#include <EditorPluginAssets/ColorGradientAsset/ColorGradientAssetWindow.moc.h>
#include <ToolsFoundation/Assets/AssetFileExtensionWhitelist.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezColorGradientAssetDocumentManager, 1, ezRTTIDefaultAllocator<ezColorGradientAssetDocumentManager>);
EZ_END_DYNAMIC_REFLECTED_TYPE

ezColorGradientAssetDocumentManager::ezColorGradientAssetDocumentManager()
{
  ezDocumentManager::s_Events.AddEventHandler(ezMakeDelegate(&ezColorGradientAssetDocumentManager::OnDocumentManagerEvent, this));

  m_AssetDesc.m_bCanCreate = true;
  m_AssetDesc.m_sDocumentTypeName = "ColorGradient Asset";
  m_AssetDesc.m_sFileExtension = "ezColorGradientAsset";
  m_AssetDesc.m_sIcon = ":/AssetIcons/ColorGradient.png";
  m_AssetDesc.m_pDocumentType = ezGetStaticRTTI<ezColorGradientAssetDocument>();
  m_AssetDesc.m_pManager = this;
}

ezColorGradientAssetDocumentManager::~ezColorGradientAssetDocumentManager()
{
  ezDocumentManager::s_Events.RemoveEventHandler(ezMakeDelegate(&ezColorGradientAssetDocumentManager::OnDocumentManagerEvent, this));
}


ezBitflags<ezAssetDocumentFlags> ezColorGradientAssetDocumentManager::GetAssetDocumentTypeFlags(const ezDocumentTypeDescriptor* pDescriptor) const
{
  EZ_ASSERT_DEBUG(pDescriptor->m_pManager == this, "Given type descriptor is not part of this document manager!");
  return ezAssetDocumentFlags::AutoTransformOnSave | ezAssetDocumentFlags::SupportsThumbnail;
}

void ezColorGradientAssetDocumentManager::OnDocumentManagerEvent(const ezDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
  case ezDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == ezGetStaticRTTI<ezColorGradientAssetDocument>())
      {
        ezColorGradientAssetDocumentWindow* pDocWnd = new ezColorGradientAssetDocumentWindow(e.m_pDocument);
      }
    }
    break;
  }
}

ezStatus ezColorGradientAssetDocumentManager::InternalCanOpenDocument(const char* szDocumentTypeName, const char* szFilePath) const
{
  return ezStatus(EZ_SUCCESS);
}

ezStatus ezColorGradientAssetDocumentManager::InternalCreateDocument(const char* szDocumentTypeName, const char* szPath, ezDocument*& out_pDocument)
{
  out_pDocument = new ezColorGradientAssetDocument(szPath);

  return ezStatus(EZ_SUCCESS);
}

void ezColorGradientAssetDocumentManager::InternalGetSupportedDocumentTypes(ezDynamicArray<const ezDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_AssetDesc);
}



