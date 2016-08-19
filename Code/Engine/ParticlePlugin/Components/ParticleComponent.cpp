#include <ParticlePlugin/PCH.h>
#include <ParticlePlugin/Components/ParticleComponent.h>
#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <ParticlePlugin/Resources/ParticleEffectResource.h>
#include <ParticlePlugin/WorldModule/ParticleWorldModule.h>
#include <Core/World/WorldModule.h>

EZ_BEGIN_COMPONENT_TYPE(ezParticleComponent, 1)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Effect", GetParticleEffectFile, SetParticleEffectFile)->AddAttributes(new ezAssetBrowserAttribute("Particle Effect")),
  }
  EZ_END_PROPERTIES
    EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("FX"),
  }
  EZ_END_ATTRIBUTES
    EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezUpdateLocalBoundsMessage, OnUpdateLocalBounds),
  }
  EZ_END_MESSAGEHANDLERS
}
EZ_END_COMPONENT_TYPE


ezParticleComponent::ezParticleComponent()
{
  m_pParticleEffect = nullptr;
}

ezParticleComponent::~ezParticleComponent()
{
  EZ_ASSERT_DEBUG(m_pParticleEffect == nullptr, "Effect pointer should have been cleared");
}

void ezParticleComponent::Initialize()
{
  if (IsActive())
  {
    GetOwner()->UpdateLocalBounds();
  }
}


void ezParticleComponent::OnAfterAttachedToObject()
{
  EZ_ASSERT_DEBUG(m_pParticleEffect == nullptr, "Invalid effect pointer");

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

  if (m_pParticleEffect)
  {
    ezParticleWorldModule* pModule = static_cast<ezParticleWorldModule*>(GetManager()->GetUserData());

    pModule->DestroyParticleEffectInstance(m_pParticleEffect);
    m_pParticleEffect = nullptr;
  }
}

void ezParticleComponent::Update()
{
  if (m_pParticleEffect == nullptr && m_hEffect.IsValid())
  {
    ezParticleWorldModule* pModule = static_cast<ezParticleWorldModule*>(GetManager()->GetUserData());

    m_pParticleEffect = pModule->CreateParticleEffectInstance(m_hEffect);
  }

  if (m_pParticleEffect)
  {
    m_pParticleEffect->SetTransform(GetOwner()->GetGlobalTransform());
  }
}

void ezParticleComponent::SerializeComponent(ezWorldWriter& stream) const
{

}

void ezParticleComponent::DeserializeComponent(ezWorldReader& stream)
{

}


void ezParticleComponent::OnUpdateLocalBounds(ezUpdateLocalBoundsMessage& msg) const
{
  // TODO
  msg.m_ResultingLocalBounds.ExpandToInclude(ezBoundingSphere(ezVec3::ZeroVector(), 3.0f));
}

void ezParticleComponent::SetParticleEffect(const ezParticleEffectResourceHandle& hEffect)
{
  m_hEffect = hEffect;

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
  if (!m_hEffect.IsValid())
    return "";

  return m_hEffect.GetResourceID();
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
