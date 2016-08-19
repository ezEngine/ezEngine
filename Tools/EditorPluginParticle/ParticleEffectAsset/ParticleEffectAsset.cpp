#include <PCH.h>
#include <EditorPluginParticle/ParticleEffectAsset/ParticleEffectAsset.h>
#include <EditorPluginParticle/ParticleEffectAsset/ParticleEffectAssetManager.h>
#include <ToolsFoundation/Reflection/PhantomRttiManager.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

//EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleEffectAssetProperties, 1, ezRTTIDefaultAllocator<ezParticleEffectAssetProperties>);
//EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleEffectAssetDocument, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE


//ezParticleEffectAssetProperties::ezParticleEffectAssetProperties()
//{
//
//}


ezParticleEffectAssetDocument::ezParticleEffectAssetDocument(const char* szDocumentPath) 
  : ezSimpleAssetDocument<ezParticleEffectDescriptor>(szDocumentPath, true)
{
}

const char* ezParticleEffectAssetDocument::QueryAssetType() const
{
  return "Particle Effect";
}


ezStatus ezParticleEffectAssetDocument::WriteParticleEffectAsset(ezStreamWriter& stream, const char* szPlatform) const
{
  const ezParticleEffectDescriptor* pProp = GetProperties();

  pProp->Save(stream);

  return ezStatus(EZ_SUCCESS);
}

void ezParticleEffectAssetDocument::UpdateAssetDocumentInfo(ezAssetDocumentInfo* pInfo) const
{
  ezAssetDocument::UpdateAssetDocumentInfo(pInfo);

  const ezParticleEffectDescriptor* pProp = GetProperties();

  /// \todo Dependencies: All asset references (color gradients etc)

}

ezStatus ezParticleEffectAssetDocument::InternalTransformAsset(ezStreamWriter& stream, const char* szPlatform, const ezAssetFileHeader& AssetHeader)
{
  return WriteParticleEffectAsset(stream, szPlatform);
}

ezStatus ezParticleEffectAssetDocument::InternalRetrieveAssetInfo(const char* szPlatform)
{
  return ezStatus(EZ_SUCCESS);
}

ezStatus ezParticleEffectAssetDocument::InternalCreateThumbnail(const ezAssetFileHeader& AssetHeader)
{
  ezStatus status = ezAssetDocument::RemoteCreateThumbnail(AssetHeader);
  return status;
}

