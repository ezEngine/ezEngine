#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Core/Curves/Curve1DResource.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamGroup.h>
#include <Foundation/Math/Float16.h>
#include <Foundation/Math/Random.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>
#include <ParticlePlugin/Initializer/ParticleInitializer_RandomRotationSpeed.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleInitializerFactory_RandomRotationSpeed, 2, ezRTTIDefaultAllocator<ezParticleInitializerFactory_RandomRotationSpeed>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("RandomStartAngle", m_bRandomStartAngle),
    EZ_MEMBER_PROPERTY("DegreesPerSecond", m_RotationSpeed)->AddAttributes(new ezDefaultValueAttribute(ezAngle::MakeFromDegree(90)), new ezClampValueAttribute(ezAngle::MakeFromDegree(0), ezVariant())),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleInitializer_RandomRotationSpeed, 1, ezRTTIDefaultAllocator<ezParticleInitializer_RandomRotationSpeed>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

const ezRTTI* ezParticleInitializerFactory_RandomRotationSpeed::GetInitializerType() const
{
  return ezGetStaticRTTI<ezParticleInitializer_RandomRotationSpeed>();
}

void ezParticleInitializerFactory_RandomRotationSpeed::CopyInitializerProperties(ezParticleInitializer* pInitializer0, bool bFirstTime) const
{
  ezParticleInitializer_RandomRotationSpeed* pInitializer = static_cast<ezParticleInitializer_RandomRotationSpeed*>(pInitializer0);

  pInitializer->m_RotationSpeed = m_RotationSpeed;
  pInitializer->m_bRandomStartAngle = m_bRandomStartAngle;
}

enum class InitializerRandomRotationVersion
{
  Version_0 = 0,
  Version_1,
  Version_2, // added start offset

  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};

void ezParticleInitializerFactory_RandomRotationSpeed::Save(ezStreamWriter& inout_stream) const
{
  const ezUInt8 uiVersion = (int)InitializerRandomRotationVersion::Version_Current;
  inout_stream << uiVersion;

  inout_stream << m_RotationSpeed.m_Value;
  inout_stream << m_RotationSpeed.m_fVariance;

  // Version 2
  inout_stream << m_bRandomStartAngle;
}

void ezParticleInitializerFactory_RandomRotationSpeed::Load(ezStreamReader& inout_stream)
{
  ezUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  inout_stream >> m_RotationSpeed.m_Value;
  inout_stream >> m_RotationSpeed.m_fVariance;

  if (uiVersion >= 2)
  {
    inout_stream >> m_bRandomStartAngle;
  }
}


void ezParticleInitializer_RandomRotationSpeed::CreateRequiredStreams()
{
  CreateStream("RotationSpeed", ezProcessingStream::DataType::Half, &m_pStreamRotationSpeed, true);
  CreateStream("RotationOffset", ezProcessingStream::DataType::Half, &m_pStreamRotationOffset, true);
}

void ezParticleInitializer_RandomRotationSpeed::InitializeElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements)
{
  EZ_PROFILE_SCOPE("PFX: Random Rotation");

  ezFloat16* pSpeed = m_pStreamRotationSpeed->GetWritableData<ezFloat16>();

  // speed
  if (m_RotationSpeed.m_Value != ezAngle::MakeFromRadian(0))
  {
    ezRandom& rng = GetRNG();

    for (ezUInt64 i = uiStartIndex; i < uiStartIndex + uiNumElements; ++i)
    {
      const float value = (float)rng.DoubleVariance(m_RotationSpeed.m_Value.GetRadian(), m_RotationSpeed.m_fVariance);

      pSpeed[i] = m_bPositiveSign ? value : -value;
      m_bPositiveSign = !m_bPositiveSign;
    }
  }
  else
  {
    for (ezUInt64 i = uiStartIndex; i < uiStartIndex + uiNumElements; ++i)
    {
      pSpeed[i] = 0;
    }
  }

  // offset
  if (m_bRandomStartAngle)
  {
    ezFloat16* pOffset = m_pStreamRotationOffset->GetWritableData<ezFloat16>();

    ezRandom& rng = GetRNG();

    for (ezUInt64 i = uiStartIndex; i < uiStartIndex + uiNumElements; ++i)
    {
      pOffset[i] = (float)rng.DoubleMinMax(-ezMath::Pi<double>(), +ezMath::Pi<double>());
    }
  }
  else
  {
    ezFloat16* pOffset = m_pStreamRotationOffset->GetWritableData<ezFloat16>();

    for (ezUInt64 i = uiStartIndex; i < uiStartIndex + uiNumElements; ++i)
    {
      pOffset[i] = 0;
    }
  }
}

//////////////////////////////////////////////////////////////////////////

class ezParticleInitializerFactory_RandomRotationSpeed_1_2 : public ezGraphPatch
{
public:
  ezParticleInitializerFactory_RandomRotationSpeed_1_2()
    : ezGraphPatch("ezParticleInitializerFactory_RandomRotationSpeed", 2)
  {
  }

  virtual void Patch(ezGraphPatchContext& ref_context, ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
  {
    pNode->InlineProperty("DegreesPerSecond").IgnoreResult();
  }
};

ezParticleInitializerFactory_RandomRotationSpeed_1_2 g_ezParticleInitializerFactory_RandomRotationSpeed_1_2;

EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Initializer_ParticleInitializer_RandomRotationSpeed);
