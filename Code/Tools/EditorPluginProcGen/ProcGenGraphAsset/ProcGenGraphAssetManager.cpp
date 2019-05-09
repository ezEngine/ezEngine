#include <EditorPluginProcGenPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorPluginProcGen/ProcGenGraphAsset/ProcGenGraphAsset.h>
#include <EditorPluginProcGen/ProcGenGraphAsset/ProcGenGraphAssetManager.h>
#include <EditorPluginProcGen/ProcGenGraphAsset/ProcGenGraphAssetWindow.moc.h>
#include <Foundation/IO/OSFile.h>
#include <GuiFoundation/UIServices/ImageCache.moc.h>
#include <ToolsFoundation/Assets/AssetFileExtensionWhitelist.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezProcGenGraphAssetDocumentManager, 1, ezRTTIDefaultAllocator<ezProcGenGraphAssetDocumentManager>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezProcGenGraphAssetDocumentManager::ezProcGenGraphAssetDocumentManager()
{
  ezDocumentManager::s_Events.AddEventHandler(ezMakeDelegate(&ezProcGenGraphAssetDocumentManager::OnDocumentManagerEvent, this));

  m_AssetDesc.m_bCanCreate = true;
  m_AssetDesc.m_sDocumentTypeName = "Procedural Placement Asset";
  m_AssetDesc.m_sFileExtension = "ezProceduralPlacementAsset";
  m_AssetDesc.m_sIcon = ":/AssetIcons/Procedural_Placement.png";
  m_AssetDesc.m_pDocumentType = ezGetStaticRTTI<ezProcGenGraphAssetDocument>();
  m_AssetDesc.m_pManager = this;

  ezQtImageCache::GetSingleton()->RegisterTypeImage("Procedural Placement", QPixmap(":/AssetIcons/Procedural_Placement.png"));
}

ezProcGenGraphAssetDocumentManager::~ezProcGenGraphAssetDocumentManager()
{
  ezDocumentManager::s_Events.RemoveEventHandler(ezMakeDelegate(&ezProcGenGraphAssetDocumentManager::OnDocumentManagerEvent, this));
}

ezBitflags<ezAssetDocumentFlags> ezProcGenGraphAssetDocumentManager::GetAssetDocumentTypeFlags(
  const ezDocumentTypeDescriptor* pDescriptor) const
{
  return ezAssetDocumentFlags::AutoTransformOnSave;
}

void ezProcGenGraphAssetDocumentManager::OnDocumentManagerEvent(const ezDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case ezDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == ezGetStaticRTTI<ezProcGenGraphAssetDocument>())
      {
        auto pDocWnd = new ezProcGenGraphAssetDocumentWindow(static_cast<ezProcGenGraphAssetDocument*>(e.m_pDocument));
      }
    }
    break;
  }
}

ezStatus ezProcGenGraphAssetDocumentManager::InternalCreateDocument(
  const char* szDocumentTypeName, const char* szPath, bool bCreateNewDocument, ezDocument*& out_pDocument)
{
  out_pDocument = new ezProcGenGraphAssetDocument(szPath);

  return ezStatus(EZ_SUCCESS);
}

void ezProcGenGraphAssetDocumentManager::InternalGetSupportedDocumentTypes(
  ezDynamicArray<const ezDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_AssetDesc);
}
