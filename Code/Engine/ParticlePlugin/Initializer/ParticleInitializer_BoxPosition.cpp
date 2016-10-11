#include <ParticlePlugin/PCH.h>
#include <ParticlePlugin/Initializer/ParticleInitializer_BoxPosition.h>
#include <CoreUtils/DataProcessing/Stream/ProcessingStreamGroup.h>
#include <Foundation/Math/Random.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleInitializerFactory_BoxPosition, 1, ezRTTIDefaultAllocator<ezParticleInitializerFactory_BoxPosition>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Size", m_vSize)->AddAttributes(new ezDefaultValueAttribute(ezVec3(0, 0, 0))),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleInitializer_BoxPosition, 1, ezRTTIDefaultAllocator<ezParticleInitializer_BoxPosition>)
EZ_END_DYNAMIC_REFLECTED_TYPE

ezParticleInitializerFactory_BoxPosition::ezParticleInitializerFactory_BoxPosition()
{
  m_vSize.Set(0, 0, 0);
}

const ezRTTI* ezParticleInitializerFactory_BoxPosition::GetInitializerType() const
{
  return ezGetStaticRTTI<ezParticleInitializer_BoxPosition>();
}

void ezParticleInitializerFactory_BoxPosition::CopyInitializerProperties(ezParticleInitializer* pInitializer0) const
{
  ezParticleInitializer_BoxPosition* pInitializer = static_cast<ezParticleInitializer_BoxPosition*>(pInitializer0);

  pInitializer->m_vSize = m_vSize.CompMax(ezVec3::ZeroVector());
}

void ezParticleInitializerFactory_BoxPosition::Save(ezStreamWriter& stream) const
{
  const ezUInt8 uiVersion = 1;
  stream << uiVersion;

  stream << m_vSize;
}

void ezParticleInitializerFactory_BoxPosition::Load(ezStreamReader& stream)
{
  ezUInt8 uiVersion = 0;
  stream >> uiVersion;

  stream >> m_vSize;
}


void ezParticleInitializer_BoxPosition::CreateRequiredStreams()
{
  CreateStream("Position", ezProcessingStream::DataType::Float3, &m_pStreamPosition, true);
}

void ezParticleInitializer_BoxPosition::InitializeElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements)
{
  ezVec3* pPosition = m_pStreamPosition->GetWritableData<ezVec3>();

  ezRandom& rng = GetRNG();

  if (m_vSize.IsZero())
  {
    const ezVec3 pos = GetOwnerSystem()->GetTransform().m_vPosition;

    for (ezUInt64 i = uiStartIndex; i < uiStartIndex + uiNumElements; ++i)
    {
      pPosition[i] = pos;
    }
  }
  else
  {
    ezVec3 pos;

    for (ezUInt64 i = uiStartIndex; i < uiStartIndex + uiNumElements; ++i)
    {
      pos.x = (float)rng.DoubleMinMax(-m_vSize.x, m_vSize.x) * 0.5f;
      pos.y = (float)rng.DoubleMinMax(-m_vSize.y, m_vSize.y) * 0.5f;
      pos.z = (float)rng.DoubleMinMax(-m_vSize.z, m_vSize.z) * 0.5f;

      pPosition[i] = GetOwnerSystem()->GetTransform() * pos;
    }
  }
}
