#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Foundation/DataProcessing/Stream/ProcessingStreamGroup.h>
#include <Foundation/Math/Random.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/SimdMath/SimdConversion.h>
#include <ParticlePlugin/Initializer/ParticleInitializer_BoxPosition.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleInitializerFactory_BoxPosition, 1, ezRTTIDefaultAllocator<ezParticleInitializerFactory_BoxPosition>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("PositionOffset", m_vPositionOffset),
    EZ_MEMBER_PROPERTY("Size", m_vSize)->AddAttributes(new ezDefaultValueAttribute(ezVec3(0, 0, 0))),
    EZ_MEMBER_PROPERTY("ScaleXParam", m_sScaleXParameter),
    EZ_MEMBER_PROPERTY("ScaleYParam", m_sScaleYParameter),
    EZ_MEMBER_PROPERTY("ScaleZParam", m_sScaleZParameter),
    EZ_MEMBER_PROPERTY("UseSpawnArea", m_bUseSpawnArea),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezBoxVisualizerAttribute("Size", 1.0f, ezColor::MediumVioletRed, nullptr, ezVisualizerAnchor::Center, ezVec3(1.0f), "PositionOffset")
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleInitializer_BoxPosition, 1, ezRTTIDefaultAllocator<ezParticleInitializer_BoxPosition>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezParticleInitializerFactory_BoxPosition::ezParticleInitializerFactory_BoxPosition()
{
  m_vPositionOffset.SetZero();
  m_vSize.Set(0, 0, 0);
}

const ezRTTI* ezParticleInitializerFactory_BoxPosition::GetInitializerType() const
{
  return ezGetStaticRTTI<ezParticleInitializer_BoxPosition>();
}

void ezParticleInitializerFactory_BoxPosition::CopyInitializerProperties(ezParticleInitializer* pInitializer0, bool bFirstTime) const
{
  ezParticleInitializer_BoxPosition* pInitializer = static_cast<ezParticleInitializer_BoxPosition*>(pInitializer0);

  const float fScaleX = pInitializer->GetOwnerEffect()->GetFloatParameter(ezTempHashedString(m_sScaleXParameter.GetData()), 1.0f);
  const float fScaleY = pInitializer->GetOwnerEffect()->GetFloatParameter(ezTempHashedString(m_sScaleYParameter.GetData()), 1.0f);
  const float fScaleZ = pInitializer->GetOwnerEffect()->GetFloatParameter(ezTempHashedString(m_sScaleZParameter.GetData()), 1.0f);

  ezVec3 vSize = m_vSize;
  vSize.x *= fScaleX;
  vSize.y *= fScaleY;
  vSize.z *= fScaleZ;

  pInitializer->m_vPositionOffset = m_vPositionOffset;
  pInitializer->m_bUseSpawnArea = m_bUseSpawnArea;
  pInitializer->m_vSize = vSize;
  pInitializer->m_vSizeScale.Set(m_vSize.x != 0.0f ? fScaleX : 0.0f, m_vSize.y != 0.0f ? fScaleY : 0.0f, m_vSize.z != 0.0f ? fScaleZ : 0.0f);
}

float ezParticleInitializerFactory_BoxPosition::GetSpawnCountMultiplier(const ezParticleEffectInstance* pEffect) const
{
  const float fScaleX = pEffect->GetFloatParameter(ezTempHashedString(m_sScaleXParameter.GetData()), 1.0f);
  const float fScaleY = pEffect->GetFloatParameter(ezTempHashedString(m_sScaleYParameter.GetData()), 1.0f);
  const float fScaleZ = pEffect->GetFloatParameter(ezTempHashedString(m_sScaleZParameter.GetData()), 1.0f);

  float fSpawnMultiplier = 1.0f;

  if (m_vSize.x != 0.0f)
    fSpawnMultiplier *= fScaleX;

  if (m_vSize.y != 0.0f)
    fSpawnMultiplier *= fScaleY;

  if (m_vSize.z != 0.0f)
    fSpawnMultiplier *= fScaleZ;

  return fSpawnMultiplier;
}

void ezParticleInitializerFactory_BoxPosition::Save(ezStreamWriter& inout_stream) const
{
  const ezUInt8 uiVersion = 4;
  inout_stream << uiVersion;

  inout_stream << m_vSize;

  // version 2
  inout_stream << m_vPositionOffset;

  // version 3
  inout_stream << m_sScaleXParameter;
  inout_stream << m_sScaleYParameter;
  inout_stream << m_sScaleZParameter;

  // version 4
  inout_stream << m_bUseSpawnArea;
}

void ezParticleInitializerFactory_BoxPosition::Load(ezStreamReader& inout_stream, const ezParticleEffectDescriptor& ownerEffectDescriptor, const ezParticleSystemDescriptor& ownerSystemDescriptor)
{
  ezUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  inout_stream >> m_vSize;

  if (uiVersion >= 2)
  {
    inout_stream >> m_vPositionOffset;
  }

  if (uiVersion >= 3)
  {
    inout_stream >> m_sScaleXParameter;
    inout_stream >> m_sScaleYParameter;
    inout_stream >> m_sScaleZParameter;
  }

  if (uiVersion >= 4)
  {
    inout_stream >> m_bUseSpawnArea;
  }
}

void ezParticleInitializer_BoxPosition::CreateRequiredStreams()
{
  CreateStream("Position", ezProcessingStream::DataType::Float4, &m_pStreamPosition, true);
}

void ezParticleInitializer_BoxPosition::OnFinalize()
{
  if (m_bUseSpawnArea)
  {
    RequestUpdateFromWorld();
  }
}

void ezParticleInitializer_BoxPosition::OnReset()
{
  m_Area.Reset();
  m_bUseArea = false;
}

void ezParticleInitializer_BoxPosition::UpdateFromWorld(const ezWorld& world, ezGameObjectHandle hOwnerObject)
{
  const ezParticleBoxAreaComponent* pArea = m_Area.Update<ezParticleBoxAreaComponent>(world, *GetOwnerEffect());

  m_bUseArea = pArea != nullptr;

  if (pArea)
  {
    m_vAreaSize = pArea->m_vSize.CompMul(m_vSizeScale);
    m_fAreaSpawnScale = pArea->m_fSpawnScale;
  }
}

bool ezParticleInitializer_BoxPosition::IsReadyToSpawn() const
{
  return !m_bUseSpawnArea || m_Area.IsReady(*GetOwnerEffect());
}

float ezParticleInitializer_BoxPosition::GetSpawnCountMultiplier() const
{
  if (!m_bUseArea)
    return 1.0f;

  // the scale of the area relative to the effect, the scale of the effect itself applies to both shapes
  const ezVec3 vAreaScale = m_Area.GetTransform().m_vScale.Abs();

  float fMultiplier = m_fAreaSpawnScale;

  for (ezUInt32 i = 0; i < 3; ++i)
  {
    if (m_vSize.GetData()[i] != 0.0f)
    {
      fMultiplier *= m_vAreaSize.GetData()[i] * vAreaScale.GetData()[i] / ezMath::Abs(m_vSize.GetData()[i]);
    }
  }

  return fMultiplier;
}

void ezParticleInitializer_BoxPosition::InitializeElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements)
{
  EZ_PROFILE_SCOPE("PFX: Box Position");

  ezSimdVec4f* pPosition = m_pStreamPosition->GetWritableData<ezSimdVec4f>();

  ezRandom& rng = GetRNG();

  // the area replaces the size, the position offset is already part of the area transform
  const ezTransform trans = m_bUseArea ? GetOwnerSystem()->GetTransform() * m_Area.GetTransformWithOffset(m_vPositionOffset) : GetOwnerSystem()->GetTransform();
  const ezVec3 vOffset = m_bUseArea ? ezVec3::MakeZero() : m_vPositionOffset;
  const ezVec3 vSize = m_bUseArea ? m_vAreaSize : m_vSize;

  if (vSize.IsZero())
  {
    ezSimdVec4f pos = ezSimdConversion::ToVec4((trans * vOffset).GetAsVec4(0));

    for (ezUInt64 i = uiStartIndex; i < uiStartIndex + uiNumElements; ++i)
    {
      pPosition[i] = pos;
    }
  }
  else
  {
    ezSimdVec4f pos;
    ezSimdTransform transform = ezSimdConversion::ToTransform(trans);

    float p0[4];
    p0[3] = 0;

    for (ezUInt64 i = uiStartIndex; i < uiStartIndex + uiNumElements; ++i)
    {
      p0[0] = (float)(rng.DoubleMinMax(-vSize.x, vSize.x) * 0.5) + vOffset.x;
      p0[1] = (float)(rng.DoubleMinMax(-vSize.y, vSize.y) * 0.5) + vOffset.y;
      p0[2] = (float)(rng.DoubleMinMax(-vSize.z, vSize.z) * 0.5) + vOffset.z;

      pos.Load<4>(p0);

      pPosition[i] = transform.TransformPosition(pos);
    }
  }
}



EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Initializer_ParticleInitializer_BoxPosition);
