#include <PCH.h>
#include <EditorPluginProceduralPlacement/ProceduralPlacementAsset/ProceduralPlacementAsset.h>
#include <EditorPluginProceduralPlacement/ProceduralPlacementAsset/ProceduralPlacementAssetManager.h>
#include <EditorPluginProceduralPlacement/ProceduralPlacementAsset/ProceduralPlacementAssetWindow.moc.h>
#include <ToolsFoundation/Assets/AssetFileExtensionWhitelist.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <GuiFoundation/UIServices/ImageCache.moc.h>
#include <Foundation/IO/OSFile.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezProceduralPlacementAssetDocumentManager, 1, ezRTTIDefaultAllocator<ezProceduralPlacementAssetDocumentManager>)
EZ_END_DYNAMIC_REFLECTED_TYPE

ezProceduralPlacementAssetDocumentManager::ezProceduralPlacementAssetDocumentManager()
{
  ezDocumentManager::s_Events.AddEventHandler(ezMakeDelegate(&ezProceduralPlacementAssetDocumentManager::OnDocumentManagerEvent, this));

  m_AssetDesc.m_bCanCreate = true;
  m_AssetDesc.m_sDocumentTypeName = "Procedural Placement Asset";
  m_AssetDesc.m_sFileExtension = "ezProceduralPlacementAsset";
  //m_AssetDesc.m_sIcon = ":/AssetIcons/Sound_Bank.png";
  m_AssetDesc.m_pDocumentType = ezGetStaticRTTI<ezProceduralPlacementAssetDocument>();
  m_AssetDesc.m_pManager = this;

  //TODO:
  //ezQtImageCache::GetSingleton()->RegisterTypeImage("Sound Bank", QPixmap(":/AssetIcons/Sound_Bank.png"));
}

ezProceduralPlacementAssetDocumentManager::~ezProceduralPlacementAssetDocumentManager()
{
  ezDocumentManager::s_Events.RemoveEventHandler(ezMakeDelegate(&ezProceduralPlacementAssetDocumentManager::OnDocumentManagerEvent, this));
}

void ezProceduralPlacementAssetDocumentManager::OnDocumentManagerEvent(const ezDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
  case ezDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == ezGetStaticRTTI<ezProceduralPlacementAssetDocument>())
      {
        ezProceduralPlacementAssetDocumentWindow* pDocWnd = new ezProceduralPlacementAssetDocumentWindow(e.m_pDocument);
      }
    }
    break;
  }
}

ezStatus ezProceduralPlacementAssetDocumentManager::InternalCreateDocument(const char* szDocumentTypeName, const char* szPath, ezDocument*& out_pDocument)
{
  out_pDocument = new ezProceduralPlacementAssetDocument(szPath);

  return ezStatus(EZ_SUCCESS);
}

void ezProceduralPlacementAssetDocumentManager::InternalGetSupportedDocumentTypes(ezDynamicArray<const ezDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_AssetDesc);
}



