#include <PCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/GUI/ExposedParameters.h>
#include <EditorPluginParticle/ParticleEffectAsset/ParticleEffectAsset.h>
#include <EditorPluginParticle/ParticleEffectAsset/ParticleEffectAssetManager.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>
#include <GuiFoundation/PropertyGrid/VisualizerManager.h>
#include <ParticlePlugin/Behavior/ParticleBehavior_ColorGradient.h>
#include <ParticlePlugin/Type/Quad/ParticleTypeQuad.h>
#include <ParticlePlugin/Type/Trail/ParticleTypeTrail.h>
#include <ToolsFoundation/Reflection/PhantomRttiManager.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleEffectAssetDocument, 4, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezParticleEffectAssetDocument::ezParticleEffectAssetDocument(const char* szDocumentPath)
    : ezSimpleAssetDocument<ezParticleEffectDescriptor>(szDocumentPath, true)
{
  ezVisualizerManager::GetSingleton()->SetVisualizersActive(this, m_bRenderVisualizers);
}

void ezParticleEffectAssetDocument::PropertyMetaStateEventHandler(ezPropertyMetaStateEvent& e)
{
  if (e.m_pObject->GetTypeAccessor().GetType() == ezGetStaticRTTI<ezParticleEffectDescriptor>())
  {
    auto& props = *e.m_pPropertyStates;

    bool bShared = e.m_pObject->GetTypeAccessor().GetValue("AlwaysShared").ConvertTo<bool>();

    props["SimulateInLocalSpace"].m_Visibility = bShared ? ezPropertyUiState::Disabled : ezPropertyUiState::Default;
    props["ApplyOwnerVelocity"].m_Visibility = bShared ? ezPropertyUiState::Disabled : ezPropertyUiState::Default;
  }
  else if (e.m_pObject->GetTypeAccessor().GetType() == ezGetStaticRTTI<ezParticleTypeQuadFactory>())
  {
    auto& props = *e.m_pPropertyStates;

    ezInt64 orientation = e.m_pObject->GetTypeAccessor().GetValue("Orientation").ConvertTo<ezInt64>();
    ezInt64 renderMode = e.m_pObject->GetTypeAccessor().GetValue("RenderMode").ConvertTo<ezInt64>();
    ezInt64 textureAtlas = e.m_pObject->GetTypeAccessor().GetValue("TextureAtlas").ConvertTo<ezInt64>();

    props["Deviation"].m_Visibility = ezPropertyUiState::Invisible;
    props["DistortionTexture"].m_Visibility = ezPropertyUiState::Invisible;
    props["DistortionStrength"].m_Visibility = ezPropertyUiState::Invisible;
    props["ParticleStretch"].m_Visibility =
        (orientation == ezQuadParticleOrientation::FixedAxis_EmitterDir || orientation == ezQuadParticleOrientation::FixedAxis_ParticleDir)
            ? ezPropertyUiState::Default
            : ezPropertyUiState::Invisible;
    props["NumSpritesX"].m_Visibility =
        (textureAtlas == (int)ezParticleTextureAtlasType::None) ? ezPropertyUiState::Invisible : ezPropertyUiState::Default;
    props["NumSpritesY"].m_Visibility =
        (textureAtlas == (int)ezParticleTextureAtlasType::None) ? ezPropertyUiState::Invisible : ezPropertyUiState::Default;


    if (orientation == ezQuadParticleOrientation::Fixed_EmitterDir || orientation == ezQuadParticleOrientation::Fixed_WorldUp)
    {
      props["Deviation"].m_Visibility = ezPropertyUiState::Default;
    }

    if (renderMode == ezParticleTypeRenderMode::Distortion)
    {
      props["DistortionTexture"].m_Visibility = ezPropertyUiState::Default;
      props["DistortionStrength"].m_Visibility = ezPropertyUiState::Default;
    }
  }
  else if (e.m_pObject->GetTypeAccessor().GetType() == ezGetStaticRTTI<ezParticleTypeTrailFactory>())
  {
    auto& props = *e.m_pPropertyStates;

    // ezInt64 renderMode = e.m_pObject->GetTypeAccessor().GetValue("RenderMode").ConvertTo<ezInt64>();
    ezInt64 textureAtlas = e.m_pObject->GetTypeAccessor().GetValue("TextureAtlas").ConvertTo<ezInt64>();

    // props["DistortionTexture"].m_Visibility = ezPropertyUiState::Invisible;
    // props["DistortionStrength"].m_Visibility = ezPropertyUiState::Invisible;
    props["NumSpritesX"].m_Visibility =
        (textureAtlas == (int)ezParticleTextureAtlasType::None) ? ezPropertyUiState::Invisible : ezPropertyUiState::Default;
    props["NumSpritesY"].m_Visibility =
        (textureAtlas == (int)ezParticleTextureAtlasType::None) ? ezPropertyUiState::Invisible : ezPropertyUiState::Default;

    // if (renderMode == ezParticleTypeRenderMode::Distortion)
    //{
    //  props["DistortionTexture"].m_Visibility = ezPropertyUiState::Default;
    //  props["DistortionStrength"].m_Visibility = ezPropertyUiState::Default;
    //}
  }
  else if (e.m_pObject->GetTypeAccessor().GetType() == ezGetStaticRTTI<ezParticleBehaviorFactory_ColorGradient>())
  {
    auto& props = *e.m_pPropertyStates;

    ezInt64 mode = e.m_pObject->GetTypeAccessor().GetValue("ColorGradientMode").ConvertTo<ezInt64>();

    props["GradientMaxSpeed"].m_Visibility =
        (mode == ezParticleColorGradientMode::Speed) ? ezPropertyUiState::Default : ezPropertyUiState::Invisible;
  }
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


void ezParticleEffectAssetDocument::SetRenderVisualizers(bool b)
{
  if (m_bRenderVisualizers == b)
    return;

  m_bRenderVisualizers = b;

  ezVisualizerManager::GetSingleton()->SetVisualizersActive(this, m_bRenderVisualizers);

  ezParticleEffectAssetEvent e;
  e.m_pDocument = this;
  e.m_Type = ezParticleEffectAssetEvent::RenderVisualizersChanged;

  m_Events.Broadcast(e);
}

ezResult ezParticleEffectAssetDocument::ComputeObjectTransformation(const ezDocumentObject* pObject, ezTransform& out_Result) const
{
  // currently the preview particle effect is always at the origin
  out_Result.SetIdentity();
  return EZ_SUCCESS;
}

void ezParticleEffectAssetDocument::UpdateAssetDocumentInfo(ezAssetDocumentInfo* pInfo) const
{
  SUPER::UpdateAssetDocumentInfo(pInfo);

  auto* desc = GetProperties();

  // shared effects do not support parameters
  if (desc->m_bAlwaysShared)
    return;

  ezExposedParameters* pExposedParams = EZ_DEFAULT_NEW(ezExposedParameters);
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

ezStatus ezParticleEffectAssetDocument::InternalTransformAsset(ezStreamWriter& stream, const char* szOutputTag, const char* szPlatform,
                                                               const ezAssetFileHeader& AssetHeader, bool bTriggeredManually)
{
  return WriteParticleEffectAsset(stream, szPlatform);
}

ezStatus ezParticleEffectAssetDocument::InternalCreateThumbnail(const ezAssetFileHeader& AssetHeader)
{
  ezStatus status = ezAssetDocument::RemoteCreateThumbnail(AssetHeader);
  return status;
}
