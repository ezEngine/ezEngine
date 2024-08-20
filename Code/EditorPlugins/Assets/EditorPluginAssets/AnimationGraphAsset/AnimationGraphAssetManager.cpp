#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/AnimationGraphAsset/AnimationGraphAsset.h>
#include <EditorPluginAssets/AnimationGraphAsset/AnimationGraphAssetManager.h>
#include <EditorPluginAssets/AnimationGraphAsset/AnimationGraphAssetWindow.moc.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAnimationGraphAssetManager, 1, ezRTTIDefaultAllocator<ezAnimationGraphAssetManager>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezAnimationGraphAssetManager::ezAnimationGraphAssetManager()
{
  ezDocumentManager::s_Events.AddEventHandler(ezMakeDelegate(&ezAnimationGraphAssetManager::OnDocumentManagerEvent, this));

  m_DocTypeDesc.m_sDocumentTypeName = "Animation Graph";
  m_DocTypeDesc.m_sFileExtension = "ezAnimationGraphAsset";
  m_DocTypeDesc.m_sIcon = ":/AssetIcons/AnimationGraph.svg";
  m_DocTypeDesc.m_sAssetCategory = "Animation";
  m_DocTypeDesc.m_pDocumentType = ezGetStaticRTTI<ezAnimationGraphAssetDocument>();
  m_DocTypeDesc.m_pManager = this;
  m_DocTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_Keyframe_Graph");

  m_DocTypeDesc.m_sResourceFileExtension = "ezBinAnimGraph";
  m_DocTypeDesc.m_AssetDocumentFlags = ezAssetDocumentFlags::AutoTransformOnSave;

  ezQtImageCache::GetSingleton()->RegisterTypeImage("Animation Graph", QPixmap(":/AssetIcons/AnimationGraph.svg"));
}

ezAnimationGraphAssetManager::~ezAnimationGraphAssetManager()
{
  ezDocumentManager::s_Events.RemoveEventHandler(ezMakeDelegate(&ezAnimationGraphAssetManager::OnDocumentManagerEvent, this));
}

void ezAnimationGraphAssetManager::OnDocumentManagerEvent(const ezDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case ezDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == ezGetStaticRTTI<ezAnimationGraphAssetDocument>())
      {
        new ezQtAnimationGraphAssetDocumentWindow(e.m_pDocument); // NOLINT: not a memory leak
      }
    }
    break;

    default:
      break;
  }
}

void ezAnimationGraphAssetManager::InternalCreateDocument(ezStringView sDocumentTypeName, ezStringView sPath, bool bCreateNewDocument, ezDocument*& out_pDocument, const ezDocumentObject* pOpenContext)
{
  out_pDocument = new ezAnimationGraphAssetDocument(sPath);
}

void ezAnimationGraphAssetManager::InternalGetSupportedDocumentTypes(ezDynamicArray<const ezDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
}
