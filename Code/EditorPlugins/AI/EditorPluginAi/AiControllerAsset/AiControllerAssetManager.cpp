#include <EditorPluginAi/EditorPluginAiPCH.h>

#include <EditorPluginAi/AiControllerAsset/AiControllerAsset.h>
#include <EditorPluginAi/AiControllerAsset/AiControllerAssetManager.h>
#include <EditorPluginAi/AiControllerAsset/AiControllerAssetWindow.moc.h>
#include <GuiFoundation/UIServices/ImageCache.moc.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAiControllerAssetManager, 1, ezRTTIDefaultAllocator<ezAiControllerAssetManager>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezAiControllerAssetManager::ezAiControllerAssetManager()
{
  ezDocumentManager::s_Events.AddEventHandler(ezMakeDelegate(&ezAiControllerAssetManager::OnDocumentManagerEvent, this));

  // the document type name has to match the name the runtime registers ezAiControllerResource with
  m_DocTypeDesc.m_sDocumentTypeName = "AI Controller";
  m_DocTypeDesc.m_sFileExtension = "ezAiControllerAsset";
  m_DocTypeDesc.m_sIcon = ":/AiPlugin/ezAiPlugin.svg";
  m_DocTypeDesc.m_sAssetCategory = "Logic";
  m_DocTypeDesc.m_pDocumentType = ezGetStaticRTTI<ezAiControllerAssetDocument>();
  m_DocTypeDesc.m_pManager = this;
  m_DocTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_AiController");

  m_DocTypeDesc.m_sResourceFileExtension = "ezBinAiController";
  m_DocTypeDesc.m_AssetDocumentFlags = ezAssetDocumentFlags::AutoTransformOnSave;

  ezQtImageCache::GetSingleton()->RegisterTypeImage("AI Controller", ezSvgThumbnailToPixmap(":/AiPlugin/ezAiPlugin.svg"));
}

ezAiControllerAssetManager::~ezAiControllerAssetManager()
{
  ezDocumentManager::s_Events.RemoveEventHandler(ezMakeDelegate(&ezAiControllerAssetManager::OnDocumentManagerEvent, this));
}

void ezAiControllerAssetManager::OnDocumentManagerEvent(const ezDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case ezDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == ezGetStaticRTTI<ezAiControllerAssetDocument>())
      {
        new ezQtAiControllerAssetDocumentWindow(e.m_pDocument); // NOLINT: not a memory leak
      }
    }
    break;

    default:
      break;
  }
}

void ezAiControllerAssetManager::InternalCreateDocument(ezStringView sDocumentTypeName, ezStringView sPath, bool bCreateNewDocument, ezDocument*& out_pDocument, const ezDocumentObject* pOpenContext)
{
  out_pDocument = new ezAiControllerAssetDocument(sPath);
}

void ezAiControllerAssetManager::InternalGetSupportedDocumentTypes(ezDynamicArray<const ezDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
}
