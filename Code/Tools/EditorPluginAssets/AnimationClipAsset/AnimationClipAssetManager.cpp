#include <EditorPluginAssetsPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorPluginAssets/AnimationClipAsset/AnimationClipAsset.h>
#include <EditorPluginAssets/AnimationClipAsset/AnimationClipAssetManager.h>
#include <EditorPluginAssets/AnimationClipAsset/AnimationClipAssetWindow.moc.h>
#include <GuiFoundation/UIServices/ImageCache.moc.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAnimationClipAssetDocumentManager, 1, ezRTTIDefaultAllocator<ezAnimationClipAssetDocumentManager>);
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezAnimationClipAssetDocumentManager::ezAnimationClipAssetDocumentManager()
{
  ezDocumentManager::s_Events.AddEventHandler(ezMakeDelegate(&ezAnimationClipAssetDocumentManager::OnDocumentManagerEvent, this));

  m_AssetDesc.m_bCanCreate = true;
  m_AssetDesc.m_sDocumentTypeName = "Animation Clip Asset";
  m_AssetDesc.m_sFileExtension = "ezAnimationClipAsset";
  m_AssetDesc.m_sIcon = ":/AssetIcons/Animation_Clip.png";
  m_AssetDesc.m_pDocumentType = ezGetStaticRTTI<ezAnimationClipAssetDocument>();
  m_AssetDesc.m_pManager = this;

  ezQtImageCache::GetSingleton()->RegisterTypeImage("Animation Clip", QPixmap(":/AssetIcons/Animation_Clip.png"));
}

ezAnimationClipAssetDocumentManager::~ezAnimationClipAssetDocumentManager()
{
  ezDocumentManager::s_Events.RemoveEventHandler(ezMakeDelegate(&ezAnimationClipAssetDocumentManager::OnDocumentManagerEvent, this));
}


ezBitflags<ezAssetDocumentFlags>
ezAnimationClipAssetDocumentManager::GetAssetDocumentTypeFlags(const ezDocumentTypeDescriptor* pDescriptor) const
{
  EZ_ASSERT_DEBUG(pDescriptor->m_pManager == this, "Given type descriptor is not part of this document manager!");
  return ezAssetDocumentFlags::None;
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
  }
}

ezStatus ezAnimationClipAssetDocumentManager::InternalCreateDocument(const char* szDocumentTypeName, const char* szPath,
                                                                     bool bCreateNewDocument, ezDocument*& out_pDocument)
{
  out_pDocument = new ezAnimationClipAssetDocument(szPath);

  return ezStatus(EZ_SUCCESS);
}

void ezAnimationClipAssetDocumentManager::InternalGetSupportedDocumentTypes(
    ezDynamicArray<const ezDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_AssetDesc);
}
