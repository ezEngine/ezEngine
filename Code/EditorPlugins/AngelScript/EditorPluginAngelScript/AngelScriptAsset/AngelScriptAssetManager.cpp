#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAngelScript/AngelScriptAsset/AngelScriptAsset.h>
#include <EditorPluginAngelScript/AngelScriptAsset/AngelScriptAssetManager.h>
#include <EditorPluginAngelScript/AngelScriptWindow/AngelScriptWindow.moc.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAngelScriptAssetManager, 1, ezRTTIDefaultAllocator<ezAngelScriptAssetManager>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezAngelScriptAssetManager::ezAngelScriptAssetManager()
{
  ezDocumentManager::s_Events.AddEventHandler(ezMakeDelegate(&ezAngelScriptAssetManager::OnDocumentManagerEvent, this));

  m_DocTypeDesc.m_sDocumentTypeName = "AngelScript";
  m_DocTypeDesc.m_sFileExtension = "ezAngelScriptAsset";
  m_DocTypeDesc.m_sIcon = ":/AssetIcons/AngelScript-AS.svg";
  m_DocTypeDesc.m_sAssetCategory = "Scripting";
  m_DocTypeDesc.m_pDocumentType = ezGetStaticRTTI<ezAngelScriptAssetDocument>();
  m_DocTypeDesc.m_pManager = this;
  m_DocTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_ScriptClass");

  m_DocTypeDesc.m_sResourceFileExtension = "ezBinAngelScript";
  m_DocTypeDesc.m_AssetDocumentFlags = ezAssetDocumentFlags::AutoTransformOnSave;

  ezQtImageCache::GetSingleton()->RegisterTypeImage("AngelScript", QPixmap(":/AssetIcons/AngelScript-Big-AS.svg"));
}

ezAngelScriptAssetManager::~ezAngelScriptAssetManager()
{
  ezDocumentManager::s_Events.RemoveEventHandler(ezMakeDelegate(&ezAngelScriptAssetManager::OnDocumentManagerEvent, this));
}

void ezAngelScriptAssetManager::OnDocumentManagerEvent(const ezDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case ezDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == ezGetStaticRTTI<ezAngelScriptAssetDocument>())
      {
        new ezQtAngelScriptAssetDocumentWindow((ezAngelScriptAssetDocument*)e.m_pDocument); // NOLINT: Not a memory leak
      }
    }
    break;

    default:
      break;
  }
}

void ezAngelScriptAssetManager::InternalCreateDocument(
  ezStringView sDocumentTypeName, ezStringView sPath, bool bCreateNewDocument, ezDocument*& out_pDocument, const ezDocumentObject* pOpenContext)
{
  out_pDocument = new ezAngelScriptAssetDocument(sPath);
}

void ezAngelScriptAssetManager::InternalGetSupportedDocumentTypes(ezDynamicArray<const ezDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
}
