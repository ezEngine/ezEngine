#include <ParticlePlugin/PCH.h>
#include <ParticlePlugin/Type/ParticleTypeEffect.h>
#include <ParticlePlugin/Resources/ParticleEffectResource.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/WorldModule/ParticleWorldModule.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleTypeEffectFactory, 1, ezRTTIDefaultAllocator<ezParticleTypeEffectFactory>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Effect", m_sEffect)->AddAttributes(new ezAssetBrowserAttribute("Particle Effect")),
    EZ_MEMBER_PROPERTY("Random Seed", m_uiRandomSeed),
    // EZ_MEMBER_PROPERTY("Shared Instance Name", m_sSharedInstanceName), // there is currently no way (I can think of) to uniquely identify each sub-system for the 'shared owner'
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleTypeEffect, 1, ezRTTIDefaultAllocator<ezParticleTypeEffect>)
EZ_END_DYNAMIC_REFLECTED_TYPE


ezParticleTypeEffectFactory::ezParticleTypeEffectFactory()
{
  m_uiRandomSeed = 0;
}

const ezRTTI* ezParticleTypeEffectFactory::GetTypeType() const
{
  return ezGetStaticRTTI<ezParticleTypeEffect>();
}

void ezParticleTypeEffectFactory::CopyTypeProperties(ezParticleType* pObject) const
{
  ezParticleTypeEffect* pType = static_cast<ezParticleTypeEffect*>(pObject);

  pType->m_hEffect.Invalidate();

  if (!m_sEffect.IsEmpty())
    pType->m_hEffect = ezResourceManager::LoadResource<ezParticleEffectResource>(m_sEffect);

  pType->m_uiRandomSeed = m_uiRandomSeed;
  pType->m_sSharedInstanceName = m_sSharedInstanceName;
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

  EZ_ASSERT_DEV(uiVersion <= (int)TypeEffectVersion::Version_Current, "Invalid version %u", uiVersion);

  stream >> m_sEffect;

  if (uiVersion >= 2)
  {
    stream >> m_uiRandomSeed;
    stream >> m_sSharedInstanceName;
  }
}


ezParticleTypeEffect::ezParticleTypeEffect()
{
}

ezParticleTypeEffect::~ezParticleTypeEffect()
{
  GetOwnerSystem()->RemoveParticleDeathEventHandler(ezMakeDelegate(&ezParticleTypeEffect::OnParticleDeath, this));

  //// delete all effects that are still in the processing group
  //{
  //  ezParticleWorldModule* pWorldModule = GetOwnerEffect()->GetOwnerWorldModule();
  //  const ezUInt64 uiNumParticles = GetOwnerSystem()->GetNumActiveParticles();

  //  ezUInt32* pEffectID = m_pStreamEffectID->GetWritableData<ezUInt32>();

  //  for (ezUInt32 elemIdx = 0; elemIdx < uiNumParticles; ++elemIdx)
  //  {
  //    ezParticleEffectHandle hInstance(pEffectID[elemIdx]);
  //    pEffectID[elemIdx] = 0;

  //    pWorldModule->DestroyParticleEffectInstance(hInstance, false, nullptr);
  //  }
  //}
}

void ezParticleTypeEffect::CreateRequiredStreams()
{
  CreateStream("Position", ezProcessingStream::DataType::Float3, &m_pStreamPosition);
  CreateStream("EffectID", ezProcessingStream::DataType::Int, &m_pStreamEffectID);
}

void ezParticleTypeEffect::AfterPropertiesConfigured(bool bFirstTime)
{
  if (bFirstTime)
  {
    GetOwnerSystem()->AddParticleDeathEventHandler(ezMakeDelegate(&ezParticleTypeEffect::OnParticleDeath, this));
  }
}

void ezParticleTypeEffect::Process(ezUInt64 uiNumElements)
{
  if (!m_hEffect.IsValid())
    return;

  const ezVec3* pPosition = m_pStreamPosition->GetData<ezVec3>();
  ezUInt32* pEffectID = m_pStreamEffectID->GetWritableData<ezUInt32>();

  ezParticleWorldModule* pWorldModule = GetOwnerEffect()->GetOwnerWorldModule();

  for (ezUInt32 i = 0; i < uiNumElements; ++i)
  {
    if (pEffectID[i] == 0) // always an invalid ID
    {
      ezParticleEffectHandle hInstance = pWorldModule->CreateParticleEffectInstance(m_hEffect, m_uiRandomSeed, m_sSharedInstanceName, nullptr);

      pEffectID[i] = hInstance.GetInternalID().m_Data;
    }

    ezParticleEffectHandle hInstance(pEffectID[i]);

    ezParticleEffectInstance* pEffect = nullptr;
    if (pWorldModule->TryGetEffect(hInstance, pEffect))
    {
      ezTransform t;
      t.m_Rotation.SetIdentity();
      t.m_vPosition = pPosition[i];

      pEffect->SetTransform(t, nullptr);
    }
  }
}

void ezParticleTypeEffect::OnParticleDeath(const ezStreamGroupElementRemovedEvent& e)
{
  ezParticleWorldModule* pWorldModule = GetOwnerEffect()->GetOwnerWorldModule();

  const ezUInt32* pEffectID = m_pStreamEffectID->GetData<ezUInt32>();

  ezParticleEffectHandle hInstance(pEffectID[e.m_uiElementIndex]);

  pWorldModule->DestroyParticleEffectInstance(hInstance, false, nullptr);
}



