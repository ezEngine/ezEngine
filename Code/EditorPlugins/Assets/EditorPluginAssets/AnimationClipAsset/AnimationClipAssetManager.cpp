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
  m_DocTypeDesc.m_sIcon = ":/AssetIcons/Animation_Clip.svg";
  m_DocTypeDesc.m_sAssetCategory = "Animation";
  m_DocTypeDesc.m_pDocumentType = ezGetStaticRTTI<ezAnimationClipAssetDocument>();
  m_DocTypeDesc.m_pManager = this;
  m_DocTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_Keyframe_Animation");

  m_DocTypeDesc.m_sResourceFileExtension = "ezBinAnimationClip";
  m_DocTypeDesc.m_AssetDocumentFlags = ezAssetDocumentFlags::SupportsThumbnail;

  // ezQtImageCache::GetSingleton()->RegisterTypeImage("Animation Clip", QPixmap(":/AssetIcons/Animation_Clip.svg"));
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
        new ezQtAnimationClipAssetDocumentWindow(static_cast<ezAnimationClipAssetDocument*>(e.m_pDocument)); // NOLINT
      }
    }
    break;

    default:
      break;
  }
}

void ezAnimationClipAssetDocumentManager::InternalCreateDocument(ezStringView sDocumentTypeName, ezStringView sPath, bool bCreateNewDocument, ezDocument*& out_pDocument, const ezDocumentObject* pOpenContext)
{
  out_pDocument = new ezAnimationClipAssetDocument(sPath);
}

void ezAnimationClipAssetDocumentManager::InternalGetSupportedDocumentTypes(
  ezDynamicArray<const ezDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
}
