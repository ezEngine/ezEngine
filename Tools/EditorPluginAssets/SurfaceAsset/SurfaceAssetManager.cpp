#include <PCH.h>
#include <EditorPluginAssets/SurfaceAsset/SurfaceAssetManager.h>
#include <EditorPluginAssets/SurfaceAsset/SurfaceAsset.h>
#include <EditorPluginAssets/SurfaceAsset/SurfaceAssetWindow.moc.h>
#include "ToolsFoundation/Assets/AssetFileExtensionWhitelist.h"

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSurfaceAssetDocumentManager, 1, ezRTTIDefaultAllocator<ezSurfaceAssetDocumentManager>);
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezSurfaceAssetDocumentManager::ezSurfaceAssetDocumentManager()
{
  ezDocumentManager::s_Events.AddEventHandler(ezMakeDelegate(&ezSurfaceAssetDocumentManager::OnDocumentManagerEvent, this));

  // additional whitelist for non-asset files where an asset may be selected
  //ezAssetFileExtensionWhitelist::AddAssetFileExtension("Surface", "ezSurface");
}

ezSurfaceAssetDocumentManager::~ezSurfaceAssetDocumentManager()
{
  ezDocumentManager::s_Events.RemoveEventHandler(ezMakeDelegate(&ezSurfaceAssetDocumentManager::OnDocumentManagerEvent, this));
}

void ezSurfaceAssetDocumentManager::OnDocumentManagerEvent(const ezDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
  case ezDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == ezGetStaticRTTI<ezSurfaceAssetDocument>())
      {
        ezSurfaceAssetDocumentWindow* pDocWnd = new ezSurfaceAssetDocumentWindow(e.m_pDocument);
      }
    }
    break;
  }
}

ezStatus ezSurfaceAssetDocumentManager::InternalCanOpenDocument(const char* szDocumentTypeName, const char* szFilePath) const
{
  return ezStatus(EZ_SUCCESS);
}

ezStatus ezSurfaceAssetDocumentManager::InternalCreateDocument(const char* szDocumentTypeName, const char* szPath, ezDocument*& out_pDocument)
{
  out_pDocument = new ezSurfaceAssetDocument(szPath);

  return ezStatus(EZ_SUCCESS);
}

void ezSurfaceAssetDocumentManager::InternalGetSupportedDocumentTypes(ezHybridArray<ezDocumentTypeDescriptor, 4>& out_DocumentTypes) const
{
  {
    ezDocumentTypeDescriptor td;
    td.m_bCanCreate = true;
    td.m_sDocumentTypeName = "Surface Asset";
    td.m_sFileExtensions.PushBack("ezSurfaceAsset");
    td.m_sIcon = ":/AssetIcons/Surface.png";

    out_DocumentTypes.PushBack(td);
  }
}



