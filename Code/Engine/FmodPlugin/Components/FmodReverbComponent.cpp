#include <FmodPlugin/PCH.h>
#include <FmodPlugin/Components/FmodReverbComponent.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <FmodPlugin/FmodSingleton.h>

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezFmodReverbPresets, 1)
  EZ_ENUM_CONSTANT(ezFmodReverbPresets::Off),
  EZ_ENUM_CONSTANT(ezFmodReverbPresets::Generic),
  EZ_ENUM_CONSTANT(ezFmodReverbPresets::PaddedCell),
  EZ_ENUM_CONSTANT(ezFmodReverbPresets::Room),
  EZ_ENUM_CONSTANT(ezFmodReverbPresets::Bathroom),
  EZ_ENUM_CONSTANT(ezFmodReverbPresets::Livingroom),
  EZ_ENUM_CONSTANT(ezFmodReverbPresets::Stoneroom),
  EZ_ENUM_CONSTANT(ezFmodReverbPresets::Auditorium),
  EZ_ENUM_CONSTANT(ezFmodReverbPresets::Concerthall),
  EZ_ENUM_CONSTANT(ezFmodReverbPresets::Cave),
  EZ_ENUM_CONSTANT(ezFmodReverbPresets::Arena),
  EZ_ENUM_CONSTANT(ezFmodReverbPresets::Hangar),
  EZ_ENUM_CONSTANT(ezFmodReverbPresets::CarpettedHallway),
  EZ_ENUM_CONSTANT(ezFmodReverbPresets::Hallway),
  EZ_ENUM_CONSTANT(ezFmodReverbPresets::StoneCorridor),
  EZ_ENUM_CONSTANT(ezFmodReverbPresets::Alley),
  EZ_ENUM_CONSTANT(ezFmodReverbPresets::Forest),
  EZ_ENUM_CONSTANT(ezFmodReverbPresets::City),
  EZ_ENUM_CONSTANT(ezFmodReverbPresets::Mountains),
  EZ_ENUM_CONSTANT(ezFmodReverbPresets::Quarry),
  EZ_ENUM_CONSTANT(ezFmodReverbPresets::Plain),
  EZ_ENUM_CONSTANT(ezFmodReverbPresets::Parkinlot),
  EZ_ENUM_CONSTANT(ezFmodReverbPresets::Sewerpipe),
  EZ_ENUM_CONSTANT(ezFmodReverbPresets::Underwater),
EZ_END_STATIC_REFLECTED_ENUM()

EZ_BEGIN_COMPONENT_TYPE(ezFmodReverbComponent, 1);
  EZ_BEGIN_PROPERTIES
    EZ_MEMBER_PROPERTY("Min Distance", m_fMinDistance)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_MEMBER_PROPERTY("Max Distance", m_fMaxDistance)->AddAttributes(new ezDefaultValueAttribute(2.0f), new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_ENUM_MEMBER_PROPERTY("Preset", ezFmodReverbPresets, m_ReverbPreset),
  EZ_END_PROPERTIES
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezFmodReverbComponent::ezFmodReverbComponent()
{
  m_pReverb = nullptr;
  m_fMinDistance = 1.0f;
  m_fMaxDistance = 2.0f;
}


void ezFmodReverbComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  auto& s = stream.GetStream();

  s << m_fMinDistance;
  s << m_fMaxDistance;

}


void ezFmodReverbComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);

  auto& s = stream.GetStream();

  s >> m_fMinDistance;
  s >> m_fMaxDistance;

}


void ezFmodReverbComponent::SetMinDistance(float f)
{
  if (m_fMinDistance == f)
    return;

  m_fMinDistance = f;

  SetParameters3d();
}


void ezFmodReverbComponent::SetMaxDistance(float f)
{
  if (m_fMaxDistance == f)
    return;

  m_fMaxDistance = f;

  SetParameters3d();
}

ezComponent::Initialization ezFmodReverbComponent::Initialize()
{
  EZ_FMOD_ASSERT(ezFmod::GetSingleton()->GetLowLevelSystem()->createReverb3D(&m_pReverb));

  SetParameters3d();

  return ezComponent::Initialization::Done;
}


void ezFmodReverbComponent::Deinitialize()
{
  if (m_pReverb != nullptr)
  {
    m_pReverb->release();
    m_pReverb = nullptr;
  }
}

void ezFmodReverbComponent::Update()
{
  /// \todo Only do this on position change

  SetParameters3d();
}

static FMOD_REVERB_PROPERTIES ReverbProperties[] =
{
  FMOD_PRESET_OFF              ,
  FMOD_PRESET_GENERIC          ,
  FMOD_PRESET_PADDEDCELL       ,
  FMOD_PRESET_ROOM             ,
  FMOD_PRESET_BATHROOM         ,
  FMOD_PRESET_LIVINGROOM       ,
  FMOD_PRESET_STONEROOM        ,
  FMOD_PRESET_AUDITORIUM       ,
  FMOD_PRESET_CONCERTHALL      ,
  FMOD_PRESET_CAVE             ,
  FMOD_PRESET_ARENA            ,
  FMOD_PRESET_HANGAR           ,
  FMOD_PRESET_CARPETTEDHALLWAY ,
  FMOD_PRESET_HALLWAY          ,
  FMOD_PRESET_STONECORRIDOR    ,
  FMOD_PRESET_ALLEY            ,
  FMOD_PRESET_FOREST           ,
  FMOD_PRESET_CITY             ,
  FMOD_PRESET_MOUNTAINS        ,
  FMOD_PRESET_QUARRY           ,
  FMOD_PRESET_PLAIN            ,
  FMOD_PRESET_PARKINGLOT       ,
  FMOD_PRESET_SEWERPIPE        ,
  FMOD_PRESET_UNDERWATER       ,
};

void ezFmodReverbComponent::SetParameters3d()
{
  if (m_pReverb == nullptr)
    return;

  const auto pos = GetOwner()->GetGlobalPosition();

  FMOD_VECTOR fmp;
  fmp.x = pos.x;
  fmp.y = pos.y;
  fmp.z = pos.z;

  EZ_FMOD_ASSERT(m_pReverb->set3DAttributes(&fmp, ezMath::Clamp(m_fMinDistance, 0.0f, m_fMaxDistance), ezMath::Max(m_fMaxDistance, ezMath::Max(0.0f, m_fMinDistance))));

  m_pReverb->setProperties(&ReverbProperties[m_ReverbPreset]);
}

