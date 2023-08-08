#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/VisualScriptAsset/VisualScriptAsset.h>
#include <EditorPluginAssets/VisualScriptAsset/VisualScriptAssetManager.h>
#include <EditorPluginAssets/VisualScriptAsset/VisualScriptAssetWindow.moc.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualScriptAssetManager, 1, ezRTTIDefaultAllocator<ezVisualScriptAssetManager>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezVisualScriptAssetManager::ezVisualScriptAssetManager()
{
  ezDocumentManager::s_Events.AddEventHandler(ezMakeDelegate(&ezVisualScriptAssetManager::OnDocumentManagerEvent, this));

  m_DocTypeDesc.m_sDocumentTypeName = "Visual Script";
  m_DocTypeDesc.m_sFileExtension = "ezVisualScriptAsset";
  m_DocTypeDesc.m_sIcon = ":/AssetIcons/Visual_Script.svg";
  m_DocTypeDesc.m_sAssetCategory = "Scripting";
  m_DocTypeDesc.m_pDocumentType = ezGetStaticRTTI<ezVisualScriptAssetDocument>();
  m_DocTypeDesc.m_pManager = this;
  m_DocTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_Code_VisualScript");

  m_DocTypeDesc.m_sResourceFileExtension = "ezVisualScriptBin";
  m_DocTypeDesc.m_AssetDocumentFlags = ezAssetDocumentFlags::AutoTransformOnSave;

  ezQtImageCache::GetSingleton()->RegisterTypeImage("Visual Script", QPixmap(":/AssetIcons/Visual_Script.svg"));
}

ezVisualScriptAssetManager::~ezVisualScriptAssetManager()
{
  ezDocumentManager::s_Events.RemoveEventHandler(ezMakeDelegate(&ezVisualScriptAssetManager::OnDocumentManagerEvent, this));
}

void ezVisualScriptAssetManager::OnDocumentManagerEvent(const ezDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case ezDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == ezGetStaticRTTI<ezVisualScriptAssetDocument>())
      {
        new ezQtVisualScriptAssetDocumentWindow(e.m_pDocument, e.m_pOpenContext); // NOLINT: Not a memory leak
      }
    }
    break;

    default:
      break;
  }
}

void ezVisualScriptAssetManager::InternalCreateDocument(
  const char* szDocumentTypeName, const char* szPath, bool bCreateNewDocument, ezDocument*& out_pDocument, const ezDocumentObject* pOpenContext)
{
  out_pDocument = new ezVisualScriptAssetDocument(szPath);
}

void ezVisualScriptAssetManager::InternalGetSupportedDocumentTypes(ezDynamicArray<const ezDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
}
