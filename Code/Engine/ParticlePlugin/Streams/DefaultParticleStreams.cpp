#include <PCH.h>

#include <Foundation/DataProcessing/Stream/ProcessingStreamIterator.h>
#include <ParticlePlugin/Streams/DefaultParticleStreams.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>
#include <Foundation/Math/Float16.h>
#include <Foundation/Math/Color16f.h>

//////////////////////////////////////////////////////////////////////////
// ZERO-INIT STREAM
//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleStream_ZeroInit, 1, ezRTTIDefaultAllocator<ezParticleStream_ZeroInit>)
EZ_END_DYNAMIC_REFLECTED_TYPE;



//////////////////////////////////////////////////////////////////////////
// POSITION STREAM
//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleStreamFactory_Position, 1, ezRTTIDefaultAllocator<ezParticleStreamFactory_Position>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleStream_Position, 1, ezRTTIDefaultAllocator<ezParticleStream_Position>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezParticleStreamFactory_Position::ezParticleStreamFactory_Position()
    : ezParticleStreamFactory("Position", ezProcessingStream::DataType::Float4, ezGetStaticRTTI<ezParticleStream_Position>())
{
}

void ezParticleStream_Position::Initialize(ezParticleSystemInstance* pOwner)
{
  m_pOwner = pOwner;
}

void ezParticleStream_Position::InitializeElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements)
{
  ezProcessingStreamIterator<ezVec4> itData(m_pStream, uiNumElements, uiStartIndex);

  const ezVec4 defValue = m_pOwner->GetTransform().m_vPosition.GetAsVec4(0);
  while (!itData.HasReachedEnd())
  {
    itData.Current() = defValue;
    itData.Advance();
  }
}

//////////////////////////////////////////////////////////////////////////
// SIZE STREAM
//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleStreamFactory_Size, 1, ezRTTIDefaultAllocator<ezParticleStreamFactory_Size>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleStream_Size, 1, ezRTTIDefaultAllocator<ezParticleStream_Size>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezParticleStreamFactory_Size::ezParticleStreamFactory_Size()
    : ezParticleStreamFactory("Size", ezProcessingStream::DataType::Half, ezGetStaticRTTI<ezParticleStream_Size>())
{
}

void ezParticleStream_Size::InitializeElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements)
{
  ezProcessingStreamIterator<ezFloat16> itData(m_pStream, uiNumElements, uiStartIndex);

  const float defValue = 1.0f;
  while (!itData.HasReachedEnd())
  {
    itData.Current() = defValue;
    itData.Advance();
  }
}

//////////////////////////////////////////////////////////////////////////
// COLOR STREAM
//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleStreamFactory_Color, 1, ezRTTIDefaultAllocator<ezParticleStreamFactory_Color>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleStream_Color, 1, ezRTTIDefaultAllocator<ezParticleStream_Color>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezParticleStreamFactory_Color::ezParticleStreamFactory_Color()
    : ezParticleStreamFactory("Color", ezProcessingStream::DataType::Half4, ezGetStaticRTTI<ezParticleStream_Color>())
{
}

void ezParticleStream_Color::InitializeElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements)
{
  ezProcessingStreamIterator<ezColorLinear16f> itData(m_pStream, uiNumElements, uiStartIndex);

  const ezColorLinear16f defValue(1.0f, 1.0f, 1.0f, 1.0f);
  while (!itData.HasReachedEnd())
  {
    itData.Current() = defValue;
    itData.Advance();
  }
}

//////////////////////////////////////////////////////////////////////////
// VELOCITY STREAM
//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleStreamFactory_Velocity, 1, ezRTTIDefaultAllocator<ezParticleStreamFactory_Velocity>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezParticleStreamFactory_Velocity::ezParticleStreamFactory_Velocity()
    : ezParticleStreamFactory("Velocity", ezProcessingStream::DataType::Float3, ezGetStaticRTTI<ezParticleStream_ZeroInit>())
{
}

//////////////////////////////////////////////////////////////////////////
// LAST POSITION STREAM
//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleStreamFactory_LastPosition, 1, ezRTTIDefaultAllocator<ezParticleStreamFactory_LastPosition>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

 ezParticleStreamFactory_LastPosition::ezParticleStreamFactory_LastPosition()
    : ezParticleStreamFactory("LastPosition", ezProcessingStream::DataType::Float3, ezGetStaticRTTI<ezParticleStream_ZeroInit>())
{
}

//////////////////////////////////////////////////////////////////////////
// ROTATION SPEED STREAM
//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleStreamFactory_RotationSpeed, 1, ezRTTIDefaultAllocator<ezParticleStreamFactory_RotationSpeed>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezParticleStreamFactory_RotationSpeed::ezParticleStreamFactory_RotationSpeed()
    : ezParticleStreamFactory("RotationSpeed", ezProcessingStream::DataType::Half, ezGetStaticRTTI<ezParticleStream_ZeroInit>())
{
}

//////////////////////////////////////////////////////////////////////////
// ROTATION OFFSET STREAM
//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleStreamFactory_RotationOffset, 1, ezRTTIDefaultAllocator<ezParticleStreamFactory_RotationOffset>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezParticleStreamFactory_RotationOffset::ezParticleStreamFactory_RotationOffset()
    : ezParticleStreamFactory("RotationOffset", ezProcessingStream::DataType::Half, ezGetStaticRTTI<ezParticleStream_ZeroInit>())
{
}

//////////////////////////////////////////////////////////////////////////
// EFFECT ID STREAM
//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleStreamFactory_EffectID, 1, ezRTTIDefaultAllocator<ezParticleStreamFactory_EffectID>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezParticleStreamFactory_EffectID::ezParticleStreamFactory_EffectID()
    : ezParticleStreamFactory("EffectID", ezProcessingStream::DataType::Int, ezGetStaticRTTI<ezParticleStream_ZeroInit>())
{
}

//////////////////////////////////////////////////////////////////////////
// ON OFF STREAM
//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleStreamFactory_OnOff, 1, ezRTTIDefaultAllocator<ezParticleStreamFactory_OnOff>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezParticleStreamFactory_OnOff::ezParticleStreamFactory_OnOff()
    : ezParticleStreamFactory("OnOff", ezProcessingStream::DataType::Int, ezGetStaticRTTI<ezParticleStream_ZeroInit>())
{
  // TODO: smaller data type
  // TODO: "Byte" type results in memory corruptions
}

//////////////////////////////////////////////////////////////////////////
// AXIS STREAM
//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleStreamFactory_Axis, 1, ezRTTIDefaultAllocator<ezParticleStreamFactory_Axis>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleStream_Axis, 1, ezRTTIDefaultAllocator<ezParticleStream_Axis>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezParticleStreamFactory_Axis::ezParticleStreamFactory_Axis()
    : ezParticleStreamFactory("Axis", ezProcessingStream::DataType::Float3, ezGetStaticRTTI<ezParticleStream_Axis>())
{
}

void ezParticleStream_Axis::InitializeElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements)
{
  ezProcessingStreamIterator<ezVec3> itData(m_pStream, uiNumElements, uiStartIndex);

  const ezVec3 defValue(1, 0, 0);
  while (!itData.HasReachedEnd())
  {
    itData.Current() = defValue;
    itData.Advance();
  }
}

//////////////////////////////////////////////////////////////////////////
// TRAIL DATA STREAM
//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleStreamFactory_TrailData, 1, ezRTTIDefaultAllocator<ezParticleStreamFactory_TrailData>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezParticleStreamFactory_TrailData::ezParticleStreamFactory_TrailData()
    : ezParticleStreamFactory("TrailData", ezProcessingStream::DataType::Short2, ezGetStaticRTTI<ezParticleStream_ZeroInit>())
{
}

EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Streams_DefaultParticleStreams);
