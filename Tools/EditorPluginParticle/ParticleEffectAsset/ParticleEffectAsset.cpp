#include <PCH.h>
#include <EditorPluginParticle/ParticleEffectAsset/ParticleEffectAsset.h>
#include <EditorPluginParticle/ParticleEffectAsset/ParticleEffectAssetManager.h>
#include <ToolsFoundation/Reflection/PhantomRttiManager.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleEffectAssetDocument, 2, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE

ezParticleEffectAssetDocument::ezParticleEffectAssetDocument(const char* szDocumentPath) 
  : ezSimpleAssetDocument<ezParticleEffectDescriptor>(szDocumentPath, true)
{
}

ezStatus ezParticleEffectAssetDocument::WriteParticleEffectAsset(ezStreamWriter& stream, const char* szPlatform) const
{
  const ezParticleEffectDescriptor* pProp = GetProperties();

  pProp->Save(stream);

  return ezStatus(EZ_SUCCESS);
}


void ezParticleEffectAssetDocument::TriggerRestartEffect()
{
  ezParticleEffectAssetEvent e;
  e.m_Type = ezParticleEffectAssetEvent::RestartEffect;

  m_Events.Broadcast(e);
}

ezStatus ezParticleEffectAssetDocument::InternalTransformAsset(ezStreamWriter& stream, const char* szPlatform, const ezAssetFileHeader& AssetHeader)
{
  return WriteParticleEffectAsset(stream, szPlatform);
}

ezStatus ezParticleEffectAssetDocument::InternalCreateThumbnail(const ezAssetFileHeader& AssetHeader)
{
  ezStatus status = ezAssetDocument::RemoteCreateThumbnail(AssetHeader);
  return status;
}


