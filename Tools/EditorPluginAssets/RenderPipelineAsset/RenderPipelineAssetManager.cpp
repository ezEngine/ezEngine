#include <PCH.h>
#include <EditorPluginAssets/RenderPipelineAsset/RenderPipelineAssetManager.h>
#include <EditorPluginAssets/RenderPipelineAsset/RenderPipelineAsset.h>
#include <EditorPluginAssets/RenderPipelineAsset/RenderPipelineAssetWindow.moc.h>
#include <ToolsFoundation/Assets/AssetFileExtensionWhitelist.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezRenderPipelineAssetManager, 1, ezRTTIDefaultAllocator<ezRenderPipelineAssetManager>);
EZ_END_DYNAMIC_REFLECTED_TYPE

ezRenderPipelineAssetManager::ezRenderPipelineAssetManager()
{
  ezDocumentManager::s_Events.AddEventHandler(ezMakeDelegate(&ezRenderPipelineAssetManager::OnDocumentManagerEvent, this));

  // additional whitelist for non-asset files where an asset may be selected
  ezAssetFileExtensionWhitelist::AddAssetFileExtension("Material", "ezMaterial");
}

ezRenderPipelineAssetManager::~ezRenderPipelineAssetManager()
{
  ezDocumentManager::s_Events.RemoveEventHandler(ezMakeDelegate(&ezRenderPipelineAssetManager::OnDocumentManagerEvent, this));
}

void ezRenderPipelineAssetManager::OnDocumentManagerEvent(const ezDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
  case ezDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == ezGetStaticRTTI<ezRenderPipelineAssetDocument>())
      {
        ezRenderPipelineAssetDocumentWindow* pDocWnd = new ezRenderPipelineAssetDocumentWindow(e.m_pDocument);
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

void ezRenderPipelineAssetManager::InternalGetSupportedDocumentTypes(ezHybridArray<ezDocumentTypeDescriptor, 4>& out_DocumentTypes) const
{
  {
    ezDocumentTypeDescriptor td;
    td.m_bCanCreate = true;
    td.m_sDocumentTypeName = "Render Pipeline Asset";
    td.m_sFileExtensions.PushBack("ezRenderPipelineAsset");
    td.m_sIcon = ":/AssetIcons/RenderPipeline.png";

    out_DocumentTypes.PushBack(td);
  }
}



