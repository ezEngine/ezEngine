#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/StateMachineAsset/StateMachineAsset.h>
#include <EditorPluginAssets/StateMachineAsset/StateMachineAssetManager.h>
#include <EditorPluginAssets/StateMachineAsset/StateMachineAssetWindow.moc.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezStateMachineAssetManager, 1, ezRTTIDefaultAllocator<ezStateMachineAssetManager>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezStateMachineAssetManager::ezStateMachineAssetManager()
{
  ezDocumentManager::s_Events.AddEventHandler(ezMakeDelegate(&ezStateMachineAssetManager::OnDocumentManagerEvent, this));

  m_DocTypeDesc.m_sDocumentTypeName = "StateMachine";
  m_DocTypeDesc.m_sFileExtension = "ezStateMachineAsset";
  m_DocTypeDesc.m_sIcon = ":/AssetIcons/StateMachine.svg";
  m_DocTypeDesc.m_sAssetCategory = "Logic";
  m_DocTypeDesc.m_pDocumentType = ezGetStaticRTTI<ezStateMachineAssetDocument>();
  m_DocTypeDesc.m_pManager = this;
  m_DocTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_StateMachine");

  m_DocTypeDesc.m_sResourceFileExtension = "ezBinStateMachine";
  m_DocTypeDesc.m_AssetDocumentFlags = ezAssetDocumentFlags::AutoTransformOnSave;

  ezQtImageCache::GetSingleton()->RegisterTypeImage("StateMachine", ezSvgThumbnailToPixmap(":/AssetIcons/StateMachine.svg"));
}

ezStateMachineAssetManager::~ezStateMachineAssetManager()
{
  ezDocumentManager::s_Events.RemoveEventHandler(ezMakeDelegate(&ezStateMachineAssetManager::OnDocumentManagerEvent, this));
}

void ezStateMachineAssetManager::OnDocumentManagerEvent(const ezDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case ezDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == ezGetStaticRTTI<ezStateMachineAssetDocument>())
      {
        new ezQtStateMachineAssetDocumentWindow(e.m_pDocument); // Not a memory leak
      }
    }
    break;

    default:
      break;
  }
}

void ezStateMachineAssetManager::InternalCreateDocument(
  ezStringView sDocumentTypeName, ezStringView sPath, bool bCreateNewDocument, ezDocument*& out_pDocument, const ezDocumentObject* pOpenContext)
{
  out_pDocument = new ezStateMachineAssetDocument(sPath);
}

void ezStateMachineAssetManager::InternalGetSupportedDocumentTypes(ezDynamicArray<const ezDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
}
