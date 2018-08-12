#include <PCH.h>

#include <EditorPluginParticle/ParticleEffectAsset/ParticleEffectAsset.h>
#include <EditorPluginParticle/ParticleEffectAsset/ParticleEffectAssetManager.h>
#include <EditorPluginParticle/ParticleEffectAsset/ParticleEffectAssetWindow.moc.h>
#include <ToolsFoundation/Assets/AssetFileExtensionWhitelist.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleEffectAssetDocumentManager, 1, ezRTTIDefaultAllocator<ezParticleEffectAssetDocumentManager>);
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezParticleEffectAssetDocumentManager::ezParticleEffectAssetDocumentManager()
{
  ezDocumentManager::s_Events.AddEventHandler(ezMakeDelegate(&ezParticleEffectAssetDocumentManager::OnDocumentManagerEvent, this));

  // additional whitelist for non-asset files where an asset may be selected
  // ezAssetFileExtensionWhitelist::AddAssetFileExtension("Collision Mesh", "ezPhysXMesh");

  m_AssetDesc.m_bCanCreate = true;
  m_AssetDesc.m_sDocumentTypeName = "Particle Effect Asset";
  m_AssetDesc.m_sFileExtension = "ezParticleEffectAsset";
  m_AssetDesc.m_sIcon = ":/AssetIcons/Particle_Effect.png";
  m_AssetDesc.m_pDocumentType = ezGetStaticRTTI<ezParticleEffectAssetDocument>();
  m_AssetDesc.m_pManager = this;
}

ezParticleEffectAssetDocumentManager::~ezParticleEffectAssetDocumentManager()
{
  ezDocumentManager::s_Events.RemoveEventHandler(ezMakeDelegate(&ezParticleEffectAssetDocumentManager::OnDocumentManagerEvent, this));
}


ezBitflags<ezAssetDocumentFlags>
ezParticleEffectAssetDocumentManager::GetAssetDocumentTypeFlags(const ezDocumentTypeDescriptor* pDescriptor) const
{
  return ezAssetDocumentFlags::AutoTransformOnSave | ezAssetDocumentFlags::SupportsThumbnail;
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

ezStatus ezParticleEffectAssetDocumentManager::InternalCreateDocument(const char* szDocumentTypeName, const char* szPath,
                                                                      ezDocument*& out_pDocument)
{
  out_pDocument = new ezParticleEffectAssetDocument(szPath);

  return ezStatus(EZ_SUCCESS);
}

void ezParticleEffectAssetDocumentManager::InternalGetSupportedDocumentTypes(
    ezDynamicArray<const ezDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_AssetDesc);
}
