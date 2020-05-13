#include <EditorPluginParticlePCH.h>

#include <EditorPluginParticle/ParticleEffectAsset/ParticleEffectAsset.h>
#include <EditorPluginParticle/ParticleEffectAsset/ParticleEffectAssetManager.h>
#include <EditorPluginParticle/ParticleEffectAsset/ParticleEffectAssetWindow.moc.h>
#include <ToolsFoundation/Assets/AssetFileExtensionWhitelist.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleEffectAssetDocumentManager, 1, ezRTTIDefaultAllocator<ezParticleEffectAssetDocumentManager>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezParticleEffectAssetDocumentManager::ezParticleEffectAssetDocumentManager()
{
  ezDocumentManager::s_Events.AddEventHandler(ezMakeDelegate(&ezParticleEffectAssetDocumentManager::OnDocumentManagerEvent, this));

  m_DocTypeDesc.m_sDocumentTypeName = "Particle Effect";
  m_DocTypeDesc.m_sFileExtension = "ezParticleEffectAsset";
  m_DocTypeDesc.m_sIcon = ":/AssetIcons/Particle_Effect.png";
  m_DocTypeDesc.m_pDocumentType = ezGetStaticRTTI<ezParticleEffectAssetDocument>();
  m_DocTypeDesc.m_pManager = this;

  m_DocTypeDesc.m_sResourceFileExtension = "ezParticleEffect";
  m_DocTypeDesc.m_AssetDocumentFlags = ezAssetDocumentFlags::AutoTransformOnSave | ezAssetDocumentFlags::SupportsThumbnail;
}

ezParticleEffectAssetDocumentManager::~ezParticleEffectAssetDocumentManager()
{
  ezDocumentManager::s_Events.RemoveEventHandler(ezMakeDelegate(&ezParticleEffectAssetDocumentManager::OnDocumentManagerEvent, this));
}

void ezParticleEffectAssetDocumentManager::OnDocumentManagerEvent(const ezDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case ezDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == ezGetStaticRTTI<ezParticleEffectAssetDocument>())
      {
        ezQtParticleEffectAssetDocumentWindow* pDocWnd =
          new ezQtParticleEffectAssetDocumentWindow(static_cast<ezParticleEffectAssetDocument*>(e.m_pDocument));
      }
    }
    break;
  }
}

void ezParticleEffectAssetDocumentManager::InternalCreateDocument(const char* szDocumentTypeName, const char* szPath, bool bCreateNewDocument, ezDocument*& out_pDocument)
{
  out_pDocument = new ezParticleEffectAssetDocument(szPath);
}

void ezParticleEffectAssetDocumentManager::InternalGetSupportedDocumentTypes(ezDynamicArray<const ezDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
}
