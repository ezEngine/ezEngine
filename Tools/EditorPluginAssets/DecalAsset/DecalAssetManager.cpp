#include <PCH.h>
#include <EditorPluginAssets/DecalAsset/DecalAssetManager.h>
#include <EditorPluginAssets/DecalAsset/DecalAsset.h>
#include <EditorPluginAssets/DecalAsset/DecalAssetWindow.moc.h>
#include "ToolsFoundation/Assets/AssetFileExtensionWhitelist.h"

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezDecalAssetDocumentManager, 1, ezRTTIDefaultAllocator<ezDecalAssetDocumentManager>);
EZ_END_DYNAMIC_REFLECTED_TYPE

ezDecalAssetDocumentManager::ezDecalAssetDocumentManager()
{
  ezDocumentManager::s_Events.AddEventHandler(ezMakeDelegate(&ezDecalAssetDocumentManager::OnDocumentManagerEvent, this));

  // texture asset source files
  ezAssetFileExtensionWhitelist::AddAssetFileExtension("Image2D", "dds");
  ezAssetFileExtensionWhitelist::AddAssetFileExtension("Image2D", "tga");

  m_AssetDesc.m_bCanCreate = true;
  m_AssetDesc.m_sDocumentTypeName = "Decal Asset";
  m_AssetDesc.m_sFileExtension = "ezDecalAsset";
  m_AssetDesc.m_sIcon = ":/AssetIcons/Decal.png";
  m_AssetDesc.m_pDocumentType = ezGetStaticRTTI<ezDecalAssetDocument>();
  m_AssetDesc.m_pManager = this;
}

ezDecalAssetDocumentManager::~ezDecalAssetDocumentManager()
{
  ezDocumentManager::s_Events.RemoveEventHandler(ezMakeDelegate(&ezDecalAssetDocumentManager::OnDocumentManagerEvent, this));
}


ezBitflags<ezAssetDocumentFlags> ezDecalAssetDocumentManager::GetAssetDocumentTypeFlags(const ezDocumentTypeDescriptor* pDescriptor) const
{
  return ezAssetDocumentFlags::Default;// ezAssetDocumentFlags::SupportsThumbnail | ezAssetDocumentFlags::AutoThumbnailOnTransform;
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

ezStatus ezDecalAssetDocumentManager::InternalCreateDocument(const char* szDocumentTypeName, const char* szPath, ezDocument*& out_pDocument)
{
  out_pDocument = new ezDecalAssetDocument(szPath);

  return ezStatus(EZ_SUCCESS);
}

void ezDecalAssetDocumentManager::InternalGetSupportedDocumentTypes(ezDynamicArray<const ezDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_AssetDesc);
}



