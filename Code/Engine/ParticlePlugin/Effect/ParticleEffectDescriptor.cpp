#include <PCH.h>
#include <ParticlePlugin/Effect/ParticleEffectDescriptor.h>
#include <ParticlePlugin/System/ParticleSystemDescriptor.h>
#include <Core/WorldSerializer/ResourceHandleWriter.h>
#include <Core/WorldSerializer/ResourceHandleReader.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleEffectDescriptor, 1, ezRTTIDefaultAllocator<ezParticleEffectDescriptor>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ENUM_MEMBER_PROPERTY("WhenInvisible", ezEffectInvisibleUpdateRate, m_InvisibleUpdateRate),
    EZ_MEMBER_PROPERTY("SimulateInLocalSpace", m_bSimulateInLocalSpace),
    EZ_MEMBER_PROPERTY("AlwaysShared", m_bAlwaysShared),
    EZ_MEMBER_PROPERTY("PreSimulateDuration", m_PreSimulateDuration),
    EZ_MAP_MEMBER_PROPERTY("FloatParameters", m_FloatParameters),
    EZ_MAP_MEMBER_PROPERTY("ColorParameters", m_ColorParameters)->AddAttributes(new ezExposeColorAlphaAttribute),
    EZ_SET_ACCESSOR_PROPERTY("ParticleSystems", GetParticleSystems, AddParticleSystem, RemoveParticleSystem)->AddFlags(ezPropertyFlags::PointerOwner),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezParticleEffectDescriptor::ezParticleEffectDescriptor()
{
}

ezParticleEffectDescriptor::~ezParticleEffectDescriptor()
{
  ClearSystems();
}

void ezParticleEffectDescriptor::ClearSystems()
{
  for (auto pSystem : m_ParticleSystems)
  {
    pSystem->GetDynamicRTTI()->GetAllocator()->Deallocate(pSystem);
  }

  m_ParticleSystems.Clear();
}

enum class ParticleEffectVersion
{
  Version_0 = 0,
  Version_1,
  Version_2,
  Version_3,
  Version_4,
  Version_5, // m_bAlwaysShared
  Version_6, // added parameters

  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};

void ezParticleEffectDescriptor::Save(ezStreamWriter& stream) const
{
  const ezUInt8 uiVersion = (int)ParticleEffectVersion::Version_Current;

  stream << uiVersion;

  const ezUInt32 uiNumSystems = m_ParticleSystems.GetCount();

  stream << uiNumSystems;

  ezResourceHandleWriteContext context;
  context.BeginWritingToStream(&stream);

  // Version 3
  stream << m_bSimulateInLocalSpace;
  stream << m_PreSimulateDuration;
  // Version 4
  stream << m_InvisibleUpdateRate;
  // Version 5
  stream << m_bAlwaysShared;

  // Version 3
  for (auto pSystem : m_ParticleSystems)
  {
    stream << pSystem->GetDynamicRTTI()->GetTypeName();

    pSystem->Save(stream);
  }

  // Version 6
  {
    ezUInt8 paramCol = m_ColorParameters.GetCount();
    stream << paramCol;
    for (auto it = m_ColorParameters.GetIterator(); it.IsValid(); ++it)
    {
      stream << it.Key();
      stream << it.Value();
    }

    ezUInt8 paramFloat = m_FloatParameters.GetCount();
    stream << paramFloat;
    for (auto it = m_FloatParameters.GetIterator(); it.IsValid(); ++it)
    {
      stream << it.Key();
      stream << it.Value();
    }
  }

  context.EndWritingToStream(&stream);
}


void ezParticleEffectDescriptor::Load(ezStreamReader& stream)
{
  ClearSystems();

  ezUInt8 uiVersion = 0;
  stream >> uiVersion;
  EZ_ASSERT_DEV(uiVersion <= (int)ParticleEffectVersion::Version_Current, "Unknown particle effect template version {0}", uiVersion);

  if (uiVersion == 1)
  {
    ezLog::SeriousWarning("Unsupported old particle effect version");
    return;
  }

  ezUInt32 uiNumSystems = 0;
  stream >> uiNumSystems;

  ezResourceHandleReadContext context;
  context.BeginReadingFromStream(&stream);
  context.BeginRestoringHandles(&stream);

  if (uiVersion >= 3)
  {
    stream >> m_bSimulateInLocalSpace;
    stream >> m_PreSimulateDuration;
  }

  if (uiVersion >= 4)
  {
    stream >> m_InvisibleUpdateRate;
  }

  if (uiVersion >= 5)
  {
    stream >> m_bAlwaysShared;
  }

  m_ParticleSystems.SetCountUninitialized(uiNumSystems);

  ezStringBuilder sType;

  for (auto& pSystem : m_ParticleSystems)
  {
    stream >> sType;

    const ezRTTI* pRtti = ezRTTI::FindTypeByName(sType);
    EZ_ASSERT_DEBUG(pRtti != nullptr, "Unknown particle effect type '{0}'", sType);

    pSystem = pRtti->GetAllocator()->Allocate<ezParticleSystemDescriptor>();

    pSystem->Load(stream);
  }

  if (uiVersion >= 6)
  {
    ezStringBuilder key;
    m_ColorParameters.Clear();
    m_FloatParameters.Clear();

    ezUInt8 paramCol;
    stream >> paramCol;
    for (ezUInt32 i = 0; i < paramCol; ++i)
    {
      ezColor val;
      stream >> key;
      stream >>val;
      m_ColorParameters[key] = val;
    }

    ezUInt8 paramFloat;
    stream >> paramFloat;
    for (ezUInt32 i = 0; i < paramFloat; ++i)
    {
      float val;
      stream >> key;
      stream >> val;
      m_FloatParameters[key] = val;
    }
  }

  context.EndReadingFromStream(&stream);
  context.EndRestoringHandles();
}



EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Effect_ParticleEffectDescriptor);

