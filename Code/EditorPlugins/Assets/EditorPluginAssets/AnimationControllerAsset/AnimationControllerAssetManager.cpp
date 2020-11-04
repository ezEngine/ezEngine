#include <EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/AnimationControllerAsset/AnimationControllerAsset.h>
#include <EditorPluginAssets/AnimationControllerAsset/AnimationControllerAssetManager.h>
#include <EditorPluginAssets/AnimationControllerAsset/AnimationControllerAssetWindow.moc.h>
#include <GuiFoundation/UIServices/ImageCache.moc.h>
#include <ToolsFoundation/Assets/AssetFileExtensionWhitelist.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAnimationControllerAssetManager, 1, ezRTTIDefaultAllocator<ezAnimationControllerAssetManager>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezAnimationControllerAssetManager::ezAnimationControllerAssetManager()
{
  ezDocumentManager::s_Events.AddEventHandler(ezMakeDelegate(&ezAnimationControllerAssetManager::OnDocumentManagerEvent, this));

  m_DocTypeDesc.m_sDocumentTypeName = "Animation Controller";
  m_DocTypeDesc.m_sFileExtension = "ezAnimationControllerAsset";
  m_DocTypeDesc.m_sIcon = ":/AssetIcons/AnimationController.png";
  m_DocTypeDesc.m_pDocumentType = ezGetStaticRTTI<ezAnimationControllerAssetDocument>();
  m_DocTypeDesc.m_pManager = this;

  m_DocTypeDesc.m_sResourceFileExtension = "ezAnimationControllerBin";
  m_DocTypeDesc.m_AssetDocumentFlags = ezAssetDocumentFlags::AutoTransformOnSave;

  ezQtImageCache::GetSingleton()->RegisterTypeImage("Animation Controller", QPixmap(":/AssetIcons/AnimationController.png"));
}

ezAnimationControllerAssetManager::~ezAnimationControllerAssetManager()
{
  ezDocumentManager::s_Events.RemoveEventHandler(ezMakeDelegate(&ezAnimationControllerAssetManager::OnDocumentManagerEvent, this));
}

void ezAnimationControllerAssetManager::OnDocumentManagerEvent(const ezDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case ezDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == ezGetStaticRTTI<ezAnimationControllerAssetDocument>())
      {
        ezQtAnimationControllerAssetDocumentWindow* pDocWnd = new ezQtAnimationControllerAssetDocumentWindow(e.m_pDocument);
      }
    }
    break;

    default:
      break;
  }
}

void ezAnimationControllerAssetManager::InternalCreateDocument(const char* szDocumentTypeName, const char* szPath, bool bCreateNewDocument, ezDocument*& out_pDocument)
{
  out_pDocument = new ezAnimationControllerAssetDocument(szPath);
}

void ezAnimationControllerAssetManager::InternalGetSupportedDocumentTypes(ezDynamicArray<const ezDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
}
