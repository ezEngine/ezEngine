#include <EditorPluginProcGenPCH.h>

#include <EditorPluginProcGen/ProcGenGraphAsset/ProcGenGraphAsset.h>
#include <EditorPluginProcGen/ProcGenGraphAsset/ProcGenGraphAssetManager.h>
#include <EditorPluginProcGen/ProcGenGraphAsset/ProcGenGraphAssetWindow.moc.h>
#include <GuiFoundation/UIServices/ImageCache.moc.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezProcGenGraphAssetDocumentManager, 1, ezRTTIDefaultAllocator<ezProcGenGraphAssetDocumentManager>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezProcGenGraphAssetDocumentManager::ezProcGenGraphAssetDocumentManager()
{
  ezDocumentManager::s_Events.AddEventHandler(ezMakeDelegate(&ezProcGenGraphAssetDocumentManager::OnDocumentManagerEvent, this));

  m_DocTypeDesc.m_sDocumentTypeName = "ProcGen Graph";
  m_DocTypeDesc.m_sFileExtension = "ezProcGenGraphAsset";
  m_DocTypeDesc.m_sIcon = ":/AssetIcons/ProcGen_Graph.png";
  m_DocTypeDesc.m_pDocumentType = ezGetStaticRTTI<ezProcGenGraphAssetDocument>();
  m_DocTypeDesc.m_pManager = this;

  m_DocTypeDesc.m_sResourceFileExtension = "ezProcGenGraph";
  m_DocTypeDesc.m_AssetDocumentFlags = ezAssetDocumentFlags::AutoTransformOnSave;

  ezQtImageCache::GetSingleton()->RegisterTypeImage("ProcGen Graph", QPixmap(":/AssetIcons/ProcGen_Graph.png"));
}

ezProcGenGraphAssetDocumentManager::~ezProcGenGraphAssetDocumentManager()
{
  ezDocumentManager::s_Events.RemoveEventHandler(ezMakeDelegate(&ezProcGenGraphAssetDocumentManager::OnDocumentManagerEvent, this));
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

    default:
      break;
  }
}

void ezProcGenGraphAssetDocumentManager::InternalCreateDocument(
  const char* szDocumentTypeName, const char* szPath, bool bCreateNewDocument, ezDocument*& out_pDocument)
{
  out_pDocument = new ezProcGenGraphAssetDocument(szPath);
}

void ezProcGenGraphAssetDocumentManager::InternalGetSupportedDocumentTypes(ezDynamicArray<const ezDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
}
