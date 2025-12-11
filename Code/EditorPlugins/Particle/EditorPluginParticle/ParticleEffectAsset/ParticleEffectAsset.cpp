#include <EditorPluginParticle/EditorPluginParticlePCH.h>

#include <EditorFramework/GUI/ExposedParameters.h>
#include <EditorPluginParticle/ParticleEffectAsset/ParticleEffectAsset.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>
#include <GuiFoundation/PropertyGrid/VisualizerManager.h>
#include <ParticlePlugin/Behavior/ParticleBehavior_ColorGradient.h>
#include <ParticlePlugin/Behavior/ParticleBehavior_Move.h>
#include <ParticlePlugin/Behavior/ParticleBehavior_Opacity.h>
#include <ParticlePlugin/Behavior/ParticleBehavior_SizeCurve.h>
#include <ParticlePlugin/Behavior/ParticleBehavior_Velocity.h>
#include <ParticlePlugin/Initializer/ParticleInitializer_CylinderPosition.h>
#include <ParticlePlugin/Initializer/ParticleInitializer_SpherePosition.h>
#include <ParticlePlugin/System/ParticleSystemDescriptor.h>
#include <ParticlePlugin/Type/Quad/ParticleTypeQuad.h>
#include <ParticlePlugin/Type/Trail/ParticleTypeTrail.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleEffectAssetDocument, 7, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezParticleEffectAssetDocument::ezParticleEffectAssetDocument(ezStringView sDocumentPath)
  : ezSimpleAssetDocument<ezParticleEffectDescriptor>(sDocumentPath, ezAssetDocEngineConnection::Simple, true)
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

    bool useMaterial = e.m_pObject->GetTypeAccessor().GetValue("UseCustomMaterial").ConvertTo<bool>();
    ezInt64 orientation = e.m_pObject->GetTypeAccessor().GetValue("Orientation").ConvertTo<ezInt64>();
    ezInt64 renderMode = e.m_pObject->GetTypeAccessor().GetValue("RenderMode").ConvertTo<ezInt64>();
    ezInt64 lightingMode = e.m_pObject->GetTypeAccessor().GetValue("LightingMode").ConvertTo<ezInt64>();
    ezInt64 textureAtlas = e.m_pObject->GetTypeAccessor().GetValue("TextureAtlas").ConvertTo<ezInt64>();

    props["Deviation"].m_Visibility = ezPropertyUiState::Invisible;
    props["DistortionTexture"].m_Visibility = ezPropertyUiState::Invisible;
    props["DistortionStrength"].m_Visibility = ezPropertyUiState::Invisible;
    props["ParticleStretch"].m_Visibility =
      (orientation == ezQuadParticleOrientation::FixedAxis_EmitterDir || orientation == ezQuadParticleOrientation::FixedAxis_ParticleDir)
        ? ezPropertyUiState::Default
        : ezPropertyUiState::Invisible;
    props["NumSpritesX"].m_Visibility = (textureAtlas == (int)ezParticleTextureAtlasType::None) ? ezPropertyUiState::Invisible : ezPropertyUiState::Default;
    props["NumSpritesY"].m_Visibility = (textureAtlas == (int)ezParticleTextureAtlasType::None) ? ezPropertyUiState::Invisible : ezPropertyUiState::Default;
    props["NormalCurvature"].m_Visibility = ezPropertyUiState::Invisible;
    props["LightDirectionality"].m_Visibility = ezPropertyUiState::Invisible;
    props["Texture"].m_Visibility = useMaterial ? ezPropertyUiState::Invisible : ezPropertyUiState::Default;
    props["CustomMaterial"].m_Visibility = useMaterial ? ezPropertyUiState::Default : ezPropertyUiState::Invisible;

    if (orientation == ezQuadParticleOrientation::Fixed_EmitterDir || orientation == ezQuadParticleOrientation::Fixed_WorldUp)
    {
      props["Deviation"].m_Visibility = ezPropertyUiState::Default;
    }

    if (lightingMode == ezParticleLightingMode::VertexLit)
    {
      props["NormalCurvature"].m_Visibility = ezPropertyUiState::Default;
      props["LightDirectionality"].m_Visibility = ezPropertyUiState::Default;
    }
  }
  else if (e.m_pObject->GetTypeAccessor().GetType() == ezGetStaticRTTI<ezParticleTypeTrailFactory>())
  {
    auto& props = *e.m_pPropertyStates;

    bool useMaterial = e.m_pObject->GetTypeAccessor().GetValue("UseCustomMaterial").ConvertTo<bool>();
    ezInt64 renderMode = e.m_pObject->GetTypeAccessor().GetValue("RenderMode").ConvertTo<ezInt64>();
    ezInt64 lightingMode = e.m_pObject->GetTypeAccessor().GetValue("LightingMode").ConvertTo<ezInt64>();
    ezInt64 textureAtlas = e.m_pObject->GetTypeAccessor().GetValue("TextureAtlas").ConvertTo<ezInt64>();

    props["DistortionTexture"].m_Visibility = ezPropertyUiState::Invisible;
    props["DistortionStrength"].m_Visibility = ezPropertyUiState::Invisible;
    props["NumSpritesX"].m_Visibility =
      (textureAtlas == (int)ezParticleTextureAtlasType::None) ? ezPropertyUiState::Invisible : ezPropertyUiState::Default;
    props["NumSpritesY"].m_Visibility =
      (textureAtlas == (int)ezParticleTextureAtlasType::None) ? ezPropertyUiState::Invisible : ezPropertyUiState::Default;
    props["NormalCurvature"].m_Visibility = ezPropertyUiState::Invisible;
    props["LightDirectionality"].m_Visibility = ezPropertyUiState::Invisible;
    props["Texture"].m_Visibility = useMaterial ? ezPropertyUiState::Invisible : ezPropertyUiState::Default;
    props["CustomMaterial"].m_Visibility = useMaterial ? ezPropertyUiState::Default : ezPropertyUiState::Invisible;

    if (lightingMode == ezParticleLightingMode::VertexLit)
    {
      props["NormalCurvature"].m_Visibility = ezPropertyUiState::Default;
      props["LightDirectionality"].m_Visibility = ezPropertyUiState::Default;
    }
  }
  else if (e.m_pObject->GetTypeAccessor().GetType() == ezGetStaticRTTI<ezParticleBehaviorFactory_ColorGradient>())
  {
    auto& props = *e.m_pPropertyStates;

    ezInt64 mode = e.m_pObject->GetTypeAccessor().GetValue("ColorGradientMode").ConvertTo<ezInt64>();

    props["GradientMaxSpeed"].m_Visibility = (mode == ezParticleColorGradientMode::Speed) ? ezPropertyUiState::Default : ezPropertyUiState::Invisible;
  }
  else if (e.m_pObject->GetTypeAccessor().GetType() == ezGetStaticRTTI<ezParticleBehaviorFactory_Opacity>())
  {
    auto& props = *e.m_pPropertyStates;

    ezInt64 curveSource = e.m_pObject->GetTypeAccessor().GetValue("ChangeOpacityWith").ConvertTo<ezInt64>();

    props["OpacityCurve"].m_Visibility = (curveSource == ezCurveSource::CustomCurve) ? ezPropertyUiState::Default : ezPropertyUiState::Invisible;
    props["SharedOpacityCurve"].m_Visibility = (curveSource == ezCurveSource::SharedCurve) ? ezPropertyUiState::Default : ezPropertyUiState::Invisible;
  }
  else if (e.m_pObject->GetTypeAccessor().GetType() == ezGetStaticRTTI<ezParticleBehaviorFactory_SizeCurve>())
  {
    auto& props = *e.m_pPropertyStates;

    ezInt64 curveSource = e.m_pObject->GetTypeAccessor().GetValue("ChangeSizeWith").ConvertTo<ezInt64>();

    props["SizeCurve"].m_Visibility = (curveSource == ezCurveSource::CustomCurve) ? ezPropertyUiState::Default : ezPropertyUiState::Invisible;
    props["SharedSizeCurve"].m_Visibility = (curveSource == ezCurveSource::SharedCurve) ? ezPropertyUiState::Default : ezPropertyUiState::Invisible;
  }
  else if (e.m_pObject->GetTypeAccessor().GetType() == ezGetStaticRTTI<ezParticleBehaviorFactory_Velocity>())
  {
    auto& props = *e.m_pPropertyStates;

    ezInt64 changeSpeedWith = e.m_pObject->GetTypeAccessor().GetValue("ChangeSpeedWith").ConvertTo<ezInt64>();

    props["Friction"].m_Visibility = (changeSpeedWith == ezVelocityChangeMode::Friction) ? ezPropertyUiState::Default : ezPropertyUiState::Invisible;
    props["SpeedCurve"].m_Visibility = (changeSpeedWith == ezVelocityChangeMode::CustomCurve) ? ezPropertyUiState::Default : ezPropertyUiState::Invisible;
    props["SharedSpeedCurve"].m_Visibility = (changeSpeedWith == ezVelocityChangeMode::SharedCurve) ? ezPropertyUiState::Default : ezPropertyUiState::Invisible;
    props["SpeedCurveOffset"].m_Visibility = (changeSpeedWith == ezVelocityChangeMode::CustomCurve || changeSpeedWith == ezVelocityChangeMode::SharedCurve) ? ezPropertyUiState::Default : ezPropertyUiState::Invisible;
    props["SpeedCurveScale"].m_Visibility = (changeSpeedWith == ezVelocityChangeMode::CustomCurve || changeSpeedWith == ezVelocityChangeMode::SharedCurve) ? ezPropertyUiState::Default : ezPropertyUiState::Invisible;
  }
  else if (e.m_pObject->GetTypeAccessor().GetType() == ezGetStaticRTTI<ezParticleBehaviorFactory_Move>())
  {
    auto& props = *e.m_pPropertyStates;

    ezInt64 moveX_Mode = e.m_pObject->GetTypeAccessor().GetValue("MoveX_Mode").ConvertTo<ezInt64>();
    ezInt64 moveY_Mode = e.m_pObject->GetTypeAccessor().GetValue("MoveY_Mode").ConvertTo<ezInt64>();
    ezInt64 moveZ_Mode = e.m_pObject->GetTypeAccessor().GetValue("MoveZ_Mode").ConvertTo<ezInt64>();

    props["MoveX_Speed"].m_Visibility = (moveX_Mode == ezMovementMode::Constant) ? ezPropertyUiState::Default : ezPropertyUiState::Invisible;
    props["MoveX_Curve"].m_Visibility = (moveX_Mode == ezMovementMode::CustomCurve) ? ezPropertyUiState::Default : ezPropertyUiState::Invisible;
    props["MoveX_SharedCurve"].m_Visibility = (moveX_Mode == ezMovementMode::SharedCurve) ? ezPropertyUiState::Default : ezPropertyUiState::Invisible;
    props["MoveX_CurveOffset"].m_Visibility = (moveX_Mode == ezMovementMode::CustomCurve || moveX_Mode == ezMovementMode::SharedCurve) ? ezPropertyUiState::Default : ezPropertyUiState::Invisible;
    props["MoveX_CurveScale"].m_Visibility = (moveX_Mode == ezMovementMode::CustomCurve || moveX_Mode == ezMovementMode::SharedCurve) ? ezPropertyUiState::Default : ezPropertyUiState::Invisible;

    props["MoveY_Speed"].m_Visibility = (moveY_Mode == ezMovementMode::Constant) ? ezPropertyUiState::Default : ezPropertyUiState::Invisible;
    props["MoveY_Curve"].m_Visibility = (moveY_Mode == ezMovementMode::CustomCurve) ? ezPropertyUiState::Default : ezPropertyUiState::Invisible;
    props["MoveY_SharedCurve"].m_Visibility = (moveY_Mode == ezMovementMode::SharedCurve) ? ezPropertyUiState::Default : ezPropertyUiState::Invisible;
    props["MoveY_CurveOffset"].m_Visibility = (moveY_Mode == ezMovementMode::CustomCurve || moveY_Mode == ezMovementMode::SharedCurve) ? ezPropertyUiState::Default : ezPropertyUiState::Invisible;
    props["MoveY_CurveScale"].m_Visibility = (moveY_Mode == ezMovementMode::CustomCurve || moveY_Mode == ezMovementMode::SharedCurve) ? ezPropertyUiState::Default : ezPropertyUiState::Invisible;

    props["MoveZ_Speed"].m_Visibility = (moveZ_Mode == ezMovementMode::Constant) ? ezPropertyUiState::Default : ezPropertyUiState::Invisible;
    props["MoveZ_Curve"].m_Visibility = (moveZ_Mode == ezMovementMode::CustomCurve) ? ezPropertyUiState::Default : ezPropertyUiState::Invisible;
    props["MoveZ_SharedCurve"].m_Visibility = (moveZ_Mode == ezMovementMode::SharedCurve) ? ezPropertyUiState::Default : ezPropertyUiState::Invisible;
    props["MoveZ_CurveOffset"].m_Visibility = (moveZ_Mode == ezMovementMode::CustomCurve || moveZ_Mode == ezMovementMode::SharedCurve) ? ezPropertyUiState::Default : ezPropertyUiState::Invisible;
    props["MoveZ_CurveScale"].m_Visibility = (moveZ_Mode == ezMovementMode::CustomCurve || moveZ_Mode == ezMovementMode::SharedCurve) ? ezPropertyUiState::Default : ezPropertyUiState::Invisible;
  }
  else if (e.m_pObject->GetTypeAccessor().GetType() == ezGetStaticRTTI<ezParticleInitializerFactory_CylinderPosition>())
  {
    auto& props = *e.m_pPropertyStates;

    bool bSetVelocity = e.m_pObject->GetTypeAccessor().GetValue("SetVelocity").ConvertTo<bool>();

    props["Speed"].m_Visibility = bSetVelocity ? ezPropertyUiState::Default : ezPropertyUiState::Invisible;
  }
  else if (e.m_pObject->GetTypeAccessor().GetType() == ezGetStaticRTTI<ezParticleInitializerFactory_SpherePosition>())
  {
    auto& props = *e.m_pPropertyStates;

    bool bSetVelocity = e.m_pObject->GetTypeAccessor().GetValue("SetVelocity").ConvertTo<bool>();

    props["Speed"].m_Visibility = bSetVelocity ? ezPropertyUiState::Default : ezPropertyUiState::Invisible;
  }
}

void ezParticleEffectAssetDocument::WriteResource(ezStreamWriter& inout_stream) const
{
  const ezParticleEffectDescriptor* pProp = GetProperties();

  pProp->Save(inout_stream);
}


void ezParticleEffectAssetDocument::TriggerRestartEffect()
{
  ezParticleEffectAssetEvent e;
  e.m_pDocument = this;
  e.m_Type = ezParticleEffectAssetEvent::RestartEffect;

  m_Events.Broadcast(e);
}


void ezParticleEffectAssetDocument::SetAutoRestart(bool bEnable)
{
  if (m_bAutoRestart == bEnable)
    return;

  m_bAutoRestart = bEnable;

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

void ezParticleEffectAssetDocument::SetSimulationSpeed(float fSpeed)
{
  if (m_fSimulationSpeed == fSpeed)
    return;

  m_fSimulationSpeed = fSpeed;

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

ezResult ezParticleEffectAssetDocument::ComputeObjectTransformation(const ezDocumentObject* pObject, ezTransform& out_result) const
{
  // currently the preview particle effect is always at the origin
  out_result.SetIdentity();
  return EZ_SUCCESS;
}

void ezParticleEffectAssetDocument::UpdateAssetDocumentInfo(ezAssetDocumentInfo* pInfo) const
{
  SUPER::UpdateAssetDocumentInfo(pInfo);

  auto* desc = GetProperties();

  for (const auto& system : desc->GetParticleSystems())
  {
    for (const auto& type : system->GetTypeFactories())
    {
      if (auto* pType = ezDynamicCast<ezParticleTypeQuadFactory*>(type))
      {
        // remove unused dependencies
        if (pType->m_bUseCustomMaterial)
        {
          pInfo->m_TransformDependencies.Remove(pType->m_sTexture);
        }
        else
        {
          pInfo->m_TransformDependencies.Remove(pType->m_sCustomMaterial);
        }
      }

      if (auto* pType = ezDynamicCast<ezParticleTypeTrailFactory*>(type))
      {
        // remove unused dependencies
        if (pType->m_bUseCustomMaterial)
        {
          pInfo->m_TransformDependencies.Remove(pType->m_sTexture);
        }
        else
        {
          pInfo->m_TransformDependencies.Remove(pType->m_sCustomMaterial);
        }
      }
    }
  }

  // shared effects do not support parameters
  if (!desc->m_bAlwaysShared)
  {
    ezExposedParameters* pExposedParams = EZ_DEFAULT_NEW(ezExposedParameters);
    for (auto it = desc->m_FloatParameters.GetIterator(); it.IsValid(); ++it)
    {
      ezExposedParameter* param = EZ_DEFAULT_NEW(ezExposedParameter);
      pExposedParams->m_Parameters.PushBack(param);
      param->m_sName = it.Key();
      param->m_DefaultValue = it.Value();
    }
    for (auto it = desc->m_ColorParameters.GetIterator(); it.IsValid(); ++it)
    {
      ezExposedParameter* param = EZ_DEFAULT_NEW(ezExposedParameter);
      pExposedParams->m_Parameters.PushBack(param);
      param->m_sName = it.Key();
      param->m_DefaultValue = it.Value();
    }

    // Info takes ownership of meta data.
    pInfo->m_MetaInfo.PushBack(pExposedParams);
  }
}

ezTransformStatus ezParticleEffectAssetDocument::InternalTransformAsset(ezStreamWriter& stream, ezStringView sOutputTag,
  const ezPlatformProfile* pAssetProfile, const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags)
{
  WriteResource(stream);
  return ezStatus(EZ_SUCCESS);
}

ezTransformStatus ezParticleEffectAssetDocument::InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo)
{
  ezStatus status = ezAssetDocument::RemoteCreateThumbnail(ThumbnailInfo);
  return status;
}
