#include <EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/RenderPipelineAsset/RenderPipelineAsset.h>
#include <EditorPluginAssets/RenderPipelineAsset/RenderPipelineAssetManager.h>
#include <EditorPluginAssets/RenderPipelineAsset/RenderPipelineAssetWindow.moc.h>
#include <GuiFoundation/UIServices/ImageCache.moc.h>
#include <ToolsFoundation/Assets/AssetFileExtensionWhitelist.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezRenderPipelineAssetManager, 1, ezRTTIDefaultAllocator<ezRenderPipelineAssetManager>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezRenderPipelineAssetManager::ezRenderPipelineAssetManager()
{
  ezDocumentManager::s_Events.AddEventHandler(ezMakeDelegate(&ezRenderPipelineAssetManager::OnDocumentManagerEvent, this));

  m_DocTypeDesc.m_sDocumentTypeName = "RenderPipeline";
  m_DocTypeDesc.m_sFileExtension = "ezRenderPipelineAsset";
  m_DocTypeDesc.m_sIcon = ":/AssetIcons/RenderPipeline.png";
  m_DocTypeDesc.m_pDocumentType = ezGetStaticRTTI<ezRenderPipelineAssetDocument>();
  m_DocTypeDesc.m_pManager = this;

  m_DocTypeDesc.m_sResourceFileExtension = "ezRenderPipelineBin";
  m_DocTypeDesc.m_AssetDocumentFlags = ezAssetDocumentFlags::AutoTransformOnSave;

  ezQtImageCache::GetSingleton()->RegisterTypeImage("RenderPipeline", QPixmap(":/AssetIcons/RenderPipeline.png"));
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
        ezQtRenderPipelineAssetDocumentWindow* pDocWnd = new ezQtRenderPipelineAssetDocumentWindow(e.m_pDocument);
      }
    }
    break;
  }
}

void ezRenderPipelineAssetManager::InternalCreateDocument(const char* szDocumentTypeName, const char* szPath, bool bCreateNewDocument, ezDocument*& out_pDocument)
{
  out_pDocument = new ezRenderPipelineAssetDocument(szPath);
}

void ezRenderPipelineAssetManager::InternalGetSupportedDocumentTypes(ezDynamicArray<const ezDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
}
