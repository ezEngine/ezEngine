#include <PCH.h>
#include <EditorPluginAssets/RenderPipelineAsset/RenderPipelineAssetManager.h>
#include <EditorPluginAssets/RenderPipelineAsset/RenderPipelineAsset.h>
#include <EditorPluginAssets/RenderPipelineAsset/RenderPipelineAssetWindow.moc.h>
#include <ToolsFoundation/Assets/AssetFileExtensionWhitelist.h>
#include <GuiFoundation/UIServices/ImageCache.moc.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezRenderPipelineAssetManager, 1, ezRTTIDefaultAllocator<ezRenderPipelineAssetManager>);
EZ_END_DYNAMIC_REFLECTED_TYPE

ezRenderPipelineAssetManager::ezRenderPipelineAssetManager()
{
  ezDocumentManager::s_Events.AddEventHandler(ezMakeDelegate(&ezRenderPipelineAssetManager::OnDocumentManagerEvent, this));

  // additional whitelist for non-asset files where an asset may be selected
  //ezAssetFileExtensionWhitelist::AddAssetFileExtension("Material", "ezMaterial");

  m_AssetDesc.m_bCanCreate = true;
  m_AssetDesc.m_sDocumentTypeName = "Render Pipeline Asset";
  m_AssetDesc.m_sFileExtension = "ezRenderPipelineAsset";
  m_AssetDesc.m_sIcon = ":/AssetIcons/RenderPipeline.png";
  m_AssetDesc.m_pDocumentType = ezGetStaticRTTI<ezRenderPipelineAssetDocument>();
  m_AssetDesc.m_pManager = this;

  ezQtImageCache::GetSingleton()->RegisterTypeImage("RenderPipeline", QPixmap(":/AssetIcons/RenderPipeline.png"));
}

ezRenderPipelineAssetManager::~ezRenderPipelineAssetManager()
{
  ezDocumentManager::s_Events.RemoveEventHandler(ezMakeDelegate(&ezRenderPipelineAssetManager::OnDocumentManagerEvent, this));
}


ezBitflags<ezAssetDocumentFlags> ezRenderPipelineAssetManager::GetAssetDocumentTypeFlags(const ezDocumentTypeDescriptor* pDescriptor) const
{
  EZ_ASSERT_DEBUG(pDescriptor->m_pManager == this, "Given type descriptor is not part of this document manager!");
  return ezAssetDocumentFlags::AutoTransformOnSave;
}

void ezRenderPipelineAssetManager::OnDocumentManagerEvent(const ezDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
  case ezDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == ezGetStaticRTTI<ezRenderPipelineAssetDocument>())
      {
        ezQtRenderPipelineAssetDocumentWindow* pDocWnd = new ezQtRenderPipelineAssetDocumentWindow(e.m_pDocument);
      }
    }
    break;
  }
}

ezStatus ezRenderPipelineAssetManager::InternalCanOpenDocument(const char* szDocumentTypeName, const char* szFilePath) const
{
  return ezStatus(EZ_SUCCESS);
}

ezStatus ezRenderPipelineAssetManager::InternalCreateDocument(const char* szDocumentTypeName, const char* szPath, ezDocument*& out_pDocument)
{
  out_pDocument = new ezRenderPipelineAssetDocument(szPath);

  return ezStatus(EZ_SUCCESS);
}

void ezRenderPipelineAssetManager::InternalGetSupportedDocumentTypes(ezDynamicArray<const ezDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_AssetDesc);
}



