#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Foundation/DataProcessing/Stream/ProcessingStreamIterator.h>
#include <Foundation/Math/Color16f.h>
#include <Foundation/Math/Float16.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/Streams/DefaultParticleStreams.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleStream_Position, 1, ezRTTIDefaultAllocator<ezParticleStream_Position>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezParticleStream_Position::ezParticleStream_Position()
  : ezParticleStream(ezParticleStreamNames::s_sPosition, ezProcessingStream::DataType::Float4)
{
}

void ezParticleStream_Position::InitializeElements(ezParticleSystemInstance* pSystem, ezProcessingStream* pStream, ezUInt64 uiStartIndex, ezUInt64 uiNumElements) const
{
  ezProcessingStreamIterator<ezVec4> itData(pStream, uiNumElements, uiStartIndex);

  const ezVec4 defValue = pSystem->GetTransform().m_vPosition.GetAsVec4(0);
  while (!itData.HasReachedEnd())
  {
    itData.Current() = defValue;
    itData.Advance();
  }
}

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleStream_Size, 1, ezRTTIDefaultAllocator<ezParticleStream_Size>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezParticleStream_Size::ezParticleStream_Size()
  : ezParticleStream(ezParticleStreamNames::s_sSize, ezProcessingStream::DataType::Half)
{
}

void ezParticleStream_Size::InitializeElements(ezParticleSystemInstance* pSystem, ezProcessingStream* pStream, ezUInt64 uiStartIndex, ezUInt64 uiNumElements) const
{
  ezProcessingStreamIterator<ezFloat16> itData(pStream, uiNumElements, uiStartIndex);

  const float defValue = 1.0f;
  while (!itData.HasReachedEnd())
  {
    itData.Current() = defValue;
    itData.Advance();
  }
}

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleStream_Color, 1, ezRTTIDefaultAllocator<ezParticleStream_Color>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezParticleStream_Color::ezParticleStream_Color()
  : ezParticleStream(ezParticleStreamNames::s_sColor, ezProcessingStream::DataType::Half4)
{
}

void ezParticleStream_Color::InitializeElements(ezParticleSystemInstance* pSystem, ezProcessingStream* pStream, ezUInt64 uiStartIndex, ezUInt64 uiNumElements) const
{
  ezProcessingStreamIterator<ezColorLinear16f> itData(pStream, uiNumElements, uiStartIndex);

  const ezColorLinear16f defValue(1.0f, 1.0f, 1.0f, 1.0f);
  while (!itData.HasReachedEnd())
  {
    itData.Current() = defValue;
    itData.Advance();
  }
}

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleStream_Velocity, 1, ezRTTIDefaultAllocator<ezParticleStream_Velocity>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezParticleStream_Velocity::ezParticleStream_Velocity()
  : ezParticleStream(ezParticleStreamNames::s_sVelocity, ezProcessingStream::DataType::Half4)
{
}

void ezParticleStream_Velocity::InitializeElements(ezParticleSystemInstance* pSystem, ezProcessingStream* pStream, ezUInt64 uiStartIndex, ezUInt64 uiNumElements) const
{
  ezProcessingStreamIterator<ezFloat16Vec4> itData(pStream, uiNumElements, uiStartIndex);

  const ezVec3 startVel = pSystem->GetParticleStartVelocity();
  const float fSpeed = startVel.GetLength();
  const ezVec3 dir = fSpeed > 0.0f ? startVel / fSpeed : ezVec3(0, 0, 1);

  while (!itData.HasReachedEnd())
  {
    itData.Current() = ezVec4(dir.x, dir.y, dir.z, fSpeed);
    itData.Advance();
  }
}

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleStream_LastPosition, 1, ezRTTIDefaultAllocator<ezParticleStream_LastPosition>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezParticleStream_LastPosition::ezParticleStream_LastPosition()
  : ezParticleStream(ezParticleStreamNames::s_sLastPosition, ezProcessingStream::DataType::Float3)
{
}

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleStream_RotationSpeed, 1, ezRTTIDefaultAllocator<ezParticleStream_RotationSpeed>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezParticleStream_RotationSpeed::ezParticleStream_RotationSpeed()
  : ezParticleStream(ezParticleStreamNames::s_sRotationSpeed, ezProcessingStream::DataType::Half)
{
}

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleStream_RotationOffset, 1, ezRTTIDefaultAllocator<ezParticleStream_RotationOffset>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezParticleStream_RotationOffset::ezParticleStream_RotationOffset()
  : ezParticleStream(ezParticleStreamNames::s_sRotationOffset, ezProcessingStream::DataType::Half)
{
}

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleStream_EffectID, 1, ezRTTIDefaultAllocator<ezParticleStream_EffectID>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezParticleStream_EffectID::ezParticleStream_EffectID()
  : ezParticleStream(ezParticleStreamNames::s_sEffectID, ezProcessingStream::DataType::Int)
{
}

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleStream_OnOff, 1, ezRTTIDefaultAllocator<ezParticleStream_OnOff>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezParticleStream_OnOff::ezParticleStream_OnOff()
  : ezParticleStream(ezParticleStreamNames::s_sOnOff, ezProcessingStream::DataType::Byte)
{
}

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleStream_Axis, 1, ezRTTIDefaultAllocator<ezParticleStream_Axis>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezParticleStream_Axis::ezParticleStream_Axis()
  : ezParticleStream(ezParticleStreamNames::s_sAxis, ezProcessingStream::DataType::Float3)
{
}

void ezParticleStream_Axis::InitializeElements(ezParticleSystemInstance* pSystem, ezProcessingStream* pStream, ezUInt64 uiStartIndex, ezUInt64 uiNumElements) const
{
  ezProcessingStreamIterator<ezVec3> itData(pStream, uiNumElements, uiStartIndex);

  const ezVec3 defValue(1, 0, 0);
  while (!itData.HasReachedEnd())
  {
    itData.Current() = defValue;
    itData.Advance();
  }
}

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleStream_TrailData, 1, ezRTTIDefaultAllocator<ezParticleStream_TrailData>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezParticleStream_TrailData::ezParticleStream_TrailData()
  : ezParticleStream(ezParticleStreamNames::s_sTrailData, ezProcessingStream::DataType::Short2)
{
}

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleStream_Variation, 1, ezRTTIDefaultAllocator<ezParticleStream_Variation>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezParticleStream_Variation::ezParticleStream_Variation()
  : ezParticleStream(ezParticleStreamNames::s_sVariation, ezProcessingStream::DataType::Int)
{
}

void ezParticleStream_Variation::InitializeElements(ezParticleSystemInstance* pSystem, ezProcessingStream* pStream, ezUInt64 uiStartIndex, ezUInt64 uiNumElements) const
{
  ezProcessingStreamIterator<ezUInt32> itData(pStream, uiNumElements, uiStartIndex);

  ezRandom& rng = pSystem->GetOwnerEffect()->GetRNG();

  while (!itData.HasReachedEnd())
  {
    itData.Current() = rng.UInt();
    itData.Advance();
  }
}


EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Streams_DefaultParticleStreams);
