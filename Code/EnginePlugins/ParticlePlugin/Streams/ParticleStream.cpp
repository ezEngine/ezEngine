#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Foundation/Memory/MemoryUtils.h>
#include <ParticlePlugin/Streams/ParticleStream.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleStream, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

const ezHashedString ezParticleStreamNames::s_sAxis = ezMakeHashedString("Axis");
const ezHashedString ezParticleStreamNames::s_sColor = ezMakeHashedString("Color");
const ezHashedString ezParticleStreamNames::s_sEffectID = ezMakeHashedString("EffectID");
const ezHashedString ezParticleStreamNames::s_sLastPosition = ezMakeHashedString("LastPosition");
const ezHashedString ezParticleStreamNames::s_sLifeTime = ezMakeHashedString("LifeTime");
const ezHashedString ezParticleStreamNames::s_sOnOff = ezMakeHashedString("OnOff");
const ezHashedString ezParticleStreamNames::s_sPosition = ezMakeHashedString("Position");
const ezHashedString ezParticleStreamNames::s_sRotationOffset = ezMakeHashedString("RotationOffset");
const ezHashedString ezParticleStreamNames::s_sRotationSpeed = ezMakeHashedString("RotationSpeed");
const ezHashedString ezParticleStreamNames::s_sSize = ezMakeHashedString("Size");
const ezHashedString ezParticleStreamNames::s_sTrailData = ezMakeHashedString("TrailData");
const ezHashedString ezParticleStreamNames::s_sVariation = ezMakeHashedString("Variation");
const ezHashedString ezParticleStreamNames::s_sVelocity = ezMakeHashedString("Velocity");

ezParticleStream::ezParticleStream(ezTempHashedString sStreamName, ezProcessingStream::DataType dataType)
{
  m_sStreamName = sStreamName;
  m_DataType = dataType;
}

ezProcessingStream::DataType ezParticleStream::GetStreamDataType() const
{
  return m_DataType;
}

ezTempHashedString ezParticleStream::GetStreamName() const
{
  return m_sStreamName;
}

void ezParticleStream::InitializeElements(ezParticleSystemInstance* pSystem, ezProcessingStream* pStream, ezUInt64 uiStartIndex, ezUInt64 uiNumElements) const
{
  ZeroFillElements(pStream, uiStartIndex, uiNumElements);
}

void ezParticleStream::ZeroFillElements(ezProcessingStream* pStream, ezUInt64 uiStartIndex, ezUInt64 uiNumElements)
{
  const ezUInt64 uiElementSize = pStream->GetElementSize();
  const ezUInt64 uiElementStride = pStream->GetElementStride();

  for (ezUInt64 i = uiStartIndex; i < uiStartIndex + uiNumElements; ++i)
  {
    ezMemoryUtils::ZeroFill<ezUInt8>(
      static_cast<ezUInt8*>(ezMemoryUtils::AddByteOffset(pStream->GetWritableData(), static_cast<ptrdiff_t>(i * uiElementStride))),
      static_cast<size_t>(uiElementSize));
  }
}

EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Streams_ParticleStream);
