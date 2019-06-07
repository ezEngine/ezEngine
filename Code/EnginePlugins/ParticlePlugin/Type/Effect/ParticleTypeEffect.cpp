#include <ParticlePluginPCH.h>

#include <Foundation/Profiling/Profiling.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/Resources/ParticleEffectResource.h>
#include <ParticlePlugin/Type/Effect/ParticleTypeEffect.h>
#include <ParticlePlugin/WorldModule/ParticleWorldModule.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleTypeEffectFactory, 1, ezRTTIDefaultAllocator<ezParticleTypeEffectFactory>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Effect", m_sEffect)->AddAttributes(new ezAssetBrowserAttribute("Particle Effect")),
    EZ_MEMBER_PROPERTY("RandomSeed", m_uiRandomSeed),
    // EZ_MEMBER_PROPERTY("Shared Instance Name", m_sSharedInstanceName), // there is currently no way (I can think of) to uniquely identify each sub-system for the 'shared owner'
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleTypeEffect, 1, ezRTTIDefaultAllocator<ezParticleTypeEffect>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezParticleTypeEffectFactory::ezParticleTypeEffectFactory()
{
  m_uiRandomSeed = 0;
}

const ezRTTI* ezParticleTypeEffectFactory::GetTypeType() const
{
  return ezGetStaticRTTI<ezParticleTypeEffect>();
}

void ezParticleTypeEffectFactory::CopyTypeProperties(ezParticleType* pObject, bool bFirstTime) const
{
  ezParticleTypeEffect* pType = static_cast<ezParticleTypeEffect*>(pObject);

  pType->m_hEffect.Invalidate();

  if (!m_sEffect.IsEmpty())
    pType->m_hEffect = ezResourceManager::LoadResource<ezParticleEffectResource>(m_sEffect);

  pType->m_uiRandomSeed = m_uiRandomSeed;
  // pType->m_sSharedInstanceName = m_sSharedInstanceName;


  if (bFirstTime)
  {
    pType->GetOwnerSystem()->AddParticleDeathEventHandler(ezMakeDelegate(&ezParticleTypeEffect::OnParticleDeath, pType));
  }
}

enum class TypeEffectVersion
{
  Version_0 = 0,
  Version_1,
  Version_2,

  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};

void ezParticleTypeEffectFactory::Save(ezStreamWriter& stream) const
{
  const ezUInt8 uiVersion = (int)TypeEffectVersion::Version_Current;
  stream << uiVersion;

  stream << m_sEffect;
  stream << m_uiRandomSeed;
  stream << m_sSharedInstanceName;
}

void ezParticleTypeEffectFactory::Load(ezStreamReader& stream)
{
  ezUInt8 uiVersion = 0;
  stream >> uiVersion;

  EZ_ASSERT_DEV(uiVersion <= (int)TypeEffectVersion::Version_Current, "Invalid version {0}", uiVersion);

  stream >> m_sEffect;

  if (uiVersion >= 2)
  {
    stream >> m_uiRandomSeed;
    stream >> m_sSharedInstanceName;
  }
}


ezParticleTypeEffect::ezParticleTypeEffect()
{
  m_pStreamEffectID = nullptr;
  m_pStreamPosition = nullptr;
}

ezParticleTypeEffect::~ezParticleTypeEffect()
{
  GetOwnerSystem()->RemoveParticleDeathEventHandler(ezMakeDelegate(&ezParticleTypeEffect::OnParticleDeath, this));

  ClearEffects(true);
}

void ezParticleTypeEffect::CreateRequiredStreams()
{
  CreateStream("Position", ezProcessingStream::DataType::Float4, &m_pStreamPosition, false);
  CreateStream("EffectID", ezProcessingStream::DataType::Int, &m_pStreamEffectID, false);
}

void ezParticleTypeEffect::OnReset()
{
  ClearEffects(true);
}

void ezParticleTypeEffect::Process(ezUInt64 uiNumElements)
{
  EZ_PROFILE_SCOPE("PFX: Effect");

  if (!m_hEffect.IsValid())
    return;

  const ezVec4* pPosition = m_pStreamPosition->GetData<ezVec4>();
  ezUInt32* pEffectID = m_pStreamEffectID->GetWritableData<ezUInt32>();

  ezParticleWorldModule* pWorldModule = GetOwnerEffect()->GetOwnerWorldModule();

  for (ezUInt32 i = 0; i < uiNumElements; ++i)
  {
    if (pEffectID[i] == 0) // always an invalid ID
    {
      const void* pDummy = nullptr;
      ezParticleEffectHandle hInstance =
          pWorldModule->CreateEffectInstance(m_hEffect, m_uiRandomSeed, /*m_sSharedInstanceName*/ nullptr, pDummy,
                                             ezArrayPtr<ezParticleEffectFloatParam>(), ezArrayPtr<ezParticleEffectColorParam>());

      pEffectID[i] = hInstance.GetInternalID().m_Data;
    }

    ezParticleEffectHandle hInstance = ezParticleEffectHandle(ezParticleEffectId(pEffectID[i]));

    ezParticleEffectInstance* pEffect = nullptr;
    if (pWorldModule->TryGetEffectInstance(hInstance, pEffect))
    {
      ezTransform t;
      t.m_qRotation.SetIdentity();
      t.m_vScale.Set(1.0f);
      t.m_vPosition = pPosition[i].GetAsVec3();

      // TODO: pass through velocity
      pEffect->SetTransform(t, ezVec3::ZeroVector(), nullptr);
    }
  }
}

void ezParticleTypeEffect::OnParticleDeath(const ezStreamGroupElementRemovedEvent& e)
{
  ezParticleWorldModule* pWorldModule = GetOwnerEffect()->GetOwnerWorldModule();

  const ezUInt32* pEffectID = m_pStreamEffectID->GetData<ezUInt32>();

  ezParticleEffectHandle hInstance = ezParticleEffectHandle(ezParticleEffectId(pEffectID[e.m_uiElementIndex]));

  pWorldModule->DestroyEffectInstance(hInstance, false, nullptr);
}

void ezParticleTypeEffect::ClearEffects(bool bInterruptImmediately)
{
  // delete all effects that are still in the processing group

  ezParticleWorldModule* pWorldModule = GetOwnerEffect()->GetOwnerWorldModule();
  const ezUInt64 uiNumParticles = GetOwnerSystem()->GetNumActiveParticles();

  if (uiNumParticles == 0 || m_pStreamEffectID == nullptr)
    return;

  ezUInt32* pEffectID = m_pStreamEffectID->GetWritableData<ezUInt32>();

  for (ezUInt32 elemIdx = 0; elemIdx < uiNumParticles; ++elemIdx)
  {
    ezParticleEffectHandle hInstance = ezParticleEffectHandle(ezParticleEffectId(pEffectID[elemIdx]));
    pEffectID[elemIdx] = 0;

    pWorldModule->DestroyEffectInstance(hInstance, bInterruptImmediately, nullptr);
  }
}

EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Type_Effect_ParticleTypeEffect);
