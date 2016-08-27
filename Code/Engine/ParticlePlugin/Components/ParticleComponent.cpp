#include <ParticlePlugin/PCH.h>
#include <ParticlePlugin/Components/ParticleComponent.h>
#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <ParticlePlugin/Resources/ParticleEffectResource.h>
#include <ParticlePlugin/WorldModule/ParticleWorldModule.h>
#include <Core/World/WorldModule.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Core/WorldSerializer/WorldReader.h>

EZ_BEGIN_COMPONENT_TYPE(ezParticleComponent, 1)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Effect", GetParticleEffectFile, SetParticleEffectFile)->AddAttributes(new ezAssetBrowserAttribute("Particle Effect")),
    EZ_MEMBER_PROPERTY("Random Seed", m_uiRandomSeed),
  }
  EZ_END_PROPERTIES
    EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("FX"),
  }
  EZ_END_ATTRIBUTES
}
EZ_END_COMPONENT_TYPE


ezParticleComponent::ezParticleComponent()
{
  m_uiRandomSeed = 0;
}

ezParticleComponent::~ezParticleComponent()
{
}

void ezParticleComponent::OnBeforeDetachedFromObject()
{
  if (IsActive()/* && m_hMesh.IsValid()*/)
  {
    // temporary set to inactive so we don't receive the msg
    SetActive(false);
    GetOwner()->UpdateLocalBounds();
    SetActive(true);
  }

  m_ParticleEffect.Invalidate();
}

void ezParticleComponent::Update()
{
  ezParticleWorldModule* pModule = static_cast<ezParticleWorldModule*>(GetManager()->GetUserData());

  if (!m_ParticleEffect.IsAlive() && m_hEffectResource.IsValid())
  {
    m_ParticleEffect.Create(m_hEffectResource, pModule, m_uiRandomSeed);
  }

  m_ParticleEffect.SetTransform(GetOwner()->GetGlobalTransform());
}

void ezParticleComponent::SerializeComponent(ezWorldWriter& stream) const
{
  auto& s = stream.GetStream();

  s << m_uiRandomSeed;
  s << m_hEffectResource;

  /// \todo store effect state
}

void ezParticleComponent::DeserializeComponent(ezWorldReader& stream)
{
  auto& s = stream.GetStream();

  s >> m_uiRandomSeed;
  s >> m_hEffectResource;

}

void ezParticleComponent::SetParticleEffect(const ezParticleEffectResourceHandle& hEffect)
{
  m_hEffectResource = hEffect;

  // TODO: Replace effect

  //if (IsActive() && GetOwner() != nullptr)
  //{
  //  GetOwner()->UpdateLocalBounds();
  //}
}


void ezParticleComponent::SetParticleEffectFile(const char* szFile)
{
  ezParticleEffectResourceHandle hEffect;

  if (!ezStringUtils::IsNullOrEmpty(szFile))
  {
    hEffect = ezResourceManager::LoadResource<ezParticleEffectResource>(szFile);
  }

  SetParticleEffect(hEffect);
}


const char* ezParticleComponent::GetParticleEffectFile() const
{
  if (!m_hEffectResource.IsValid())
    return "";

  return m_hEffectResource.GetResourceID();
}


ezResult ezParticleComponent::GetLocalBounds(ezBoundingBoxSphere& bounds)
{
  bounds.ExpandToInclude(ezBoundingSphere(ezVec3::ZeroVector(), 1.0f));
  return EZ_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////



ezParticleComponentManager::ezParticleComponentManager(ezWorld* pWorld)
  : ezComponentManagerSimple<ComponentType, true>(pWorld)
{
}

void ezParticleComponentManager::Initialize()
{
  ezComponentManagerSimple<ComponentType, true>::Initialize();

  ezParticleWorldModule* pModule = static_cast<ezParticleWorldModule*>(ezWorldModule::FindModule(GetWorld(), ezParticleWorldModule::GetStaticRTTI()));

  SetUserData(pModule);
}
