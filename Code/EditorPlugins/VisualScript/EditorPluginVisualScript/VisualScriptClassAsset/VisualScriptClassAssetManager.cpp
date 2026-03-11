#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginVisualScript/VisualScriptClassAsset/VisualScriptClassAsset.h>
#include <EditorPluginVisualScript/VisualScriptClassAsset/VisualScriptClassAssetManager.h>
#include <EditorPluginVisualScript/VisualScriptGraph/VisualScriptWindow.moc.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualScriptClassAssetManager, 1, ezRTTIDefaultAllocator<ezVisualScriptClassAssetManager>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezVisualScriptClassAssetManager::ezVisualScriptClassAssetManager()
{
  ezDocumentManager::s_Events.AddEventHandler(ezMakeDelegate(&ezVisualScriptClassAssetManager::OnDocumentManagerEvent, this));

  m_DocTypeDesc.m_sDocumentTypeName = "VisualScriptClass";
  m_DocTypeDesc.m_sFileExtension = "ezVisualScriptClassAsset";
  m_DocTypeDesc.m_sIcon = ":/AssetIcons/VisualScript.svg";
  m_DocTypeDesc.m_sAssetCategory = "Scripting";
  m_DocTypeDesc.m_pDocumentType = ezGetStaticRTTI<ezVisualScriptClassAssetDocument>();
  m_DocTypeDesc.m_pManager = this;
  m_DocTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_ScriptClass");

  m_DocTypeDesc.m_sResourceFileExtension = "ezBinVisualScriptClass";
  m_DocTypeDesc.m_AssetDocumentFlags = ezAssetDocumentFlags::AutoTransformOnSave;

  ezQtImageCache::GetSingleton()->RegisterTypeImage("VisualScriptClass", QPixmap(":/AssetIcons/VisualScript.svg"));
}

ezVisualScriptClassAssetManager::~ezVisualScriptClassAssetManager()
{
  ezDocumentManager::s_Events.RemoveEventHandler(ezMakeDelegate(&ezVisualScriptClassAssetManager::OnDocumentManagerEvent, this));
}

void ezVisualScriptClassAssetManager::OnDocumentManagerEvent(const ezDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case ezDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == ezGetStaticRTTI<ezVisualScriptClassAssetDocument>())
      {
        new ezQtVisualScriptWindow(e.m_pDocument); // NOLINT: Not a memory leak
      }
    }
    break;

    default:
      break;
  }
}

void ezVisualScriptClassAssetManager::InternalCreateDocument(
  ezStringView sDocumentTypeName, ezStringView sPath, bool bCreateNewDocument, ezDocument*& out_pDocument, const ezDocumentObject* pOpenContext)
{
  out_pDocument = new ezVisualScriptClassAssetDocument(sPath);
}

void ezVisualScriptClassAssetManager::InternalGetSupportedDocumentTypes(ezDynamicArray<const ezDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
}
