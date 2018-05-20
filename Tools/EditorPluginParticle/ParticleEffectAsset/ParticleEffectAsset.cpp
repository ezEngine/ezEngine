#include <PCH.h>
#include <EditorPluginParticle/ParticleEffectAsset/ParticleEffectAsset.h>
#include <EditorPluginParticle/ParticleEffectAsset/ParticleEffectAssetManager.h>
#include <ToolsFoundation/Reflection/PhantomRttiManager.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>
#include <EditorFramework/GUI/ExposedParameters.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleEffectAssetDocument, 3, ezRTTINoAllocator);
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
  e.m_pDocument = this;
  e.m_Type = ezParticleEffectAssetEvent::RestartEffect;

  m_Events.Broadcast(e);
}


void ezParticleEffectAssetDocument::SetAutoRestart(bool enable)
{
  if (m_bAutoRestart == enable)
    return;

  m_bAutoRestart = enable;

  ezParticleEffectAssetEvent e;
  e.m_pDocument = this;
  e.m_Type = ezParticleEffectAssetEvent::AutoRestartChanged;

  m_Events.Broadcast(e);
}


void ezParticleEffectAssetDocument::SetSimulationPaused(bool bPaused)
{
  if (m_bSimulationPaused == bPaused)
    return;

  m_bSimulationPaused = bPaused;

  ezParticleEffectAssetEvent e;
  e.m_pDocument = this;
  e.m_Type = ezParticleEffectAssetEvent::SimulationSpeedChanged;

  m_Events.Broadcast(e);
}

void ezParticleEffectAssetDocument::SetSimulationSpeed(float speed)
{
  if (m_fSimulationSpeed == speed)
    return;

  m_fSimulationSpeed = speed;

  ezParticleEffectAssetEvent e;
  e.m_pDocument = this;
  e.m_Type = ezParticleEffectAssetEvent::SimulationSpeedChanged;

  m_Events.Broadcast(e);
}

void ezParticleEffectAssetDocument::UpdateAssetDocumentInfo(ezAssetDocumentInfo* pInfo) const
{
  SUPER::UpdateAssetDocumentInfo(pInfo);

  ezExposedParameters* pExposedParams = EZ_DEFAULT_NEW(ezExposedParameters);
  auto* desc = GetProperties();
  for (auto it = desc->m_FloatParameters.GetIterator(); it.IsValid(); ++it)
  {
    auto& param = pExposedParams->m_Parameters.ExpandAndGetRef();
    param.m_sName = it.Key();
    param.m_DefaultValue = it.Value();
  }
  for (auto it = desc->m_ColorParameters.GetIterator(); it.IsValid(); ++it)
  {
    auto& param = pExposedParams->m_Parameters.ExpandAndGetRef();
    param.m_sName = it.Key();
    param.m_DefaultValue = it.Value();
  }

  // Info takes ownership of meta data.
  pInfo->m_MetaInfo.PushBack(pExposedParams);
}

ezStatus ezParticleEffectAssetDocument::InternalTransformAsset(ezStreamWriter& stream, const char* szOutputTag, const char* szPlatform, const ezAssetFileHeader& AssetHeader, bool bTriggeredManually)
{
  return WriteParticleEffectAsset(stream, szPlatform);
}

ezStatus ezParticleEffectAssetDocument::InternalCreateThumbnail(const ezAssetFileHeader& AssetHeader)
{
  ezStatus status = ezAssetDocument::RemoteCreateThumbnail(AssetHeader);
  return status;
}


