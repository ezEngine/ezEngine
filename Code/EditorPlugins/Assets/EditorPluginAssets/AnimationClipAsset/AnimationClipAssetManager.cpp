#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorPluginAssets/AnimationClipAsset/AnimationClipAsset.h>
#include <EditorPluginAssets/AnimationClipAsset/AnimationClipAssetManager.h>
#include <EditorPluginAssets/AnimationClipAsset/AnimationClipAssetWindow.moc.h>
#include <GuiFoundation/UIServices/ImageCache.moc.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAnimationClipAssetDocumentManager, 1, ezRTTIDefaultAllocator<ezAnimationClipAssetDocumentManager>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezAnimationClipAssetDocumentManager::ezAnimationClipAssetDocumentManager()
{
  ezDocumentManager::s_Events.AddEventHandler(ezMakeDelegate(&ezAnimationClipAssetDocumentManager::OnDocumentManagerEvent, this));

  m_DocTypeDesc.m_sDocumentTypeName = "Animation Clip";
  m_DocTypeDesc.m_sFileExtension = "ezAnimationClipAsset";
  m_DocTypeDesc.m_sIcon = ":/AssetIcons/Animation_Clip.png";
  m_DocTypeDesc.m_pDocumentType = ezGetStaticRTTI<ezAnimationClipAssetDocument>();
  m_DocTypeDesc.m_pManager = this;

  m_DocTypeDesc.m_sResourceFileExtension = "ezAnimationClip";
  m_DocTypeDesc.m_AssetDocumentFlags = ezAssetDocumentFlags::SupportsThumbnail;

  //ezQtImageCache::GetSingleton()->RegisterTypeImage("Animation Clip", QPixmap(":/AssetIcons/Animation_Clip.png"));
}

ezAnimationClipAssetDocumentManager::~ezAnimationClipAssetDocumentManager()
{
  ezDocumentManager::s_Events.RemoveEventHandler(ezMakeDelegate(&ezAnimationClipAssetDocumentManager::OnDocumentManagerEvent, this));
}

void ezAnimationClipAssetDocumentManager::OnDocumentManagerEvent(const ezDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case ezDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == ezGetStaticRTTI<ezAnimationClipAssetDocument>())
      {
        ezQtAnimationClipAssetDocumentWindow* pDocWnd =
          new ezQtAnimationClipAssetDocumentWindow(static_cast<ezAnimationClipAssetDocument*>(e.m_pDocument));
      }
    }
    break;

    default:
      break;
  }
}

void ezAnimationClipAssetDocumentManager::InternalCreateDocument(
  const char* szDocumentTypeName, const char* szPath, bool bCreateNewDocument, ezDocument*& out_pDocument)
{
  out_pDocument = new ezAnimationClipAssetDocument(szPath);
}

void ezAnimationClipAssetDocumentManager::InternalGetSupportedDocumentTypes(
  ezDynamicArray<const ezDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
}
