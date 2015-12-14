#include <PCH.h>
#include <EditorPluginFmod/SoundBankAsset/SoundBankAssetManager.h>
#include <EditorPluginFmod/SoundBankAsset/SoundBankAsset.h>
#include <EditorPluginFmod/SoundBankAsset/SoundBankAssetWindow.moc.h>
#include "ToolsFoundation/Assets/AssetFileExtensionWhitelist.h"

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSoundBankAssetDocumentManager, 1, ezRTTIDefaultAllocator<ezSoundBankAssetDocumentManager>);
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezSoundBankAssetDocumentManager::ezSoundBankAssetDocumentManager()
{
  ezDocumentManager::s_Events.AddEventHandler(ezMakeDelegate(&ezSoundBankAssetDocumentManager::OnDocumentManagerEvent, this));

  // additional whitelist for non-asset files where an asset may be selected
  //ezAssetFileExtensionWhitelist::AddAssetFileExtension("Collision Mesh", "ezFmodMesh");

}

ezSoundBankAssetDocumentManager::~ezSoundBankAssetDocumentManager()
{
  ezDocumentManager::s_Events.RemoveEventHandler(ezMakeDelegate(&ezSoundBankAssetDocumentManager::OnDocumentManagerEvent, this));
}

void ezSoundBankAssetDocumentManager::OnDocumentManagerEvent(const ezDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
  case ezDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == ezGetStaticRTTI<ezSoundBankAssetDocument>())
      {
        ezSoundBankAssetDocumentWindow* pDocWnd = new ezSoundBankAssetDocumentWindow(e.m_pDocument);
      }
    }
    break;
  }
}

ezStatus ezSoundBankAssetDocumentManager::InternalCanOpenDocument(const char* szDocumentTypeName, const char* szFilePath) const
{
  return EZ_SUCCESS;
}

ezStatus ezSoundBankAssetDocumentManager::InternalCreateDocument(const char* szDocumentTypeName, const char* szPath, ezDocument*& out_pDocument)
{
  out_pDocument = new ezSoundBankAssetDocument(szPath);

  return ezStatus(EZ_SUCCESS);
}

void ezSoundBankAssetDocumentManager::InternalGetSupportedDocumentTypes(ezHybridArray<ezDocumentTypeDescriptor, 4>& out_DocumentTypes) const
{
  {
    ezDocumentTypeDescriptor td;
    td.m_bCanCreate = true;
    td.m_sDocumentTypeName = "Sound Bank Asset";
    td.m_sFileExtensions.PushBack("ezSoundBankAsset");
    td.m_sIcon = ":/AssetIcons/Sound_Bank.png";

    out_DocumentTypes.PushBack(td);
  }
}



