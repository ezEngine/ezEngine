#include <EditorPluginProcGen/EditorPluginProcGenPCH.h>

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
  m_DocTypeDesc.m_sIcon = ":/AssetIcons/ProcGen_Graph.svg";
  m_DocTypeDesc.m_sAssetCategory = "Construction";
  m_DocTypeDesc.m_pDocumentType = ezGetStaticRTTI<ezProcGenGraphAssetDocument>();
  m_DocTypeDesc.m_pManager = this;
  m_DocTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_ProcGen_Graph");

  m_DocTypeDesc.m_sResourceFileExtension = "ezBinProcGenGraph";
  m_DocTypeDesc.m_AssetDocumentFlags = ezAssetDocumentFlags::AutoTransformOnSave;

  ezQtImageCache::GetSingleton()->RegisterTypeImage("ProcGen Graph", QPixmap(":/AssetIcons/ProcGen_Graph.svg"));
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
        new ezProcGenGraphAssetDocumentWindow(static_cast<ezProcGenGraphAssetDocument*>(e.m_pDocument)); // NOLINT: Not a memory leak
      }
    }
    break;

    default:
      break;
  }
}

void ezProcGenGraphAssetDocumentManager::InternalCreateDocument(
  ezStringView sDocumentTypeName, ezStringView sPath, bool bCreateNewDocument, ezDocument*& out_pDocument, const ezDocumentObject* pOpenContext)
{
  out_pDocument = new ezProcGenGraphAssetDocument(sPath);
}

void ezProcGenGraphAssetDocumentManager::InternalGetSupportedDocumentTypes(ezDynamicArray<const ezDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
}
