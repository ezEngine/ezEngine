#include <ParticlePlugin/PCH.h>
#include <ParticlePlugin/Streams/DefaultParticleStreams.h>
#include <CoreUtils/DataProcessing/Stream/ProcessingStreamIterator.h>

//////////////////////////////////////////////////////////////////////////
// POSITION STREAM
//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleStreamFactory_Position, 1, ezRTTIDefaultAllocator<ezParticleStreamFactory_Position>)
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleStream_Position, 1, ezRTTIDefaultAllocator<ezParticleStream_Position>)
EZ_END_DYNAMIC_REFLECTED_TYPE

const ezRTTI* ezParticleStreamFactory_Position::GetParticleStreamType() const
{
  return ezGetStaticRTTI<ezParticleStream_Position>();
}

const char* ezParticleStreamFactory_Position::GetStreamName() const
{
  return "Position";
}

ezProcessingStream::DataType ezParticleStreamFactory_Position::GetStreamDataType() const
{
  return ezProcessingStream::DataType::Float3;
}

//////////////////////////////////////////////////////////////////////////
// SIZE STREAM
//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleStreamFactory_Size, 1, ezRTTIDefaultAllocator<ezParticleStreamFactory_Size>)
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleStream_Size, 1, ezRTTIDefaultAllocator<ezParticleStream_Size>)
EZ_END_DYNAMIC_REFLECTED_TYPE

const ezRTTI* ezParticleStreamFactory_Size::GetParticleStreamType() const
{
  return ezGetStaticRTTI<ezParticleStream_Size>();
}

const char* ezParticleStreamFactory_Size::GetStreamName() const
{
  return "Size";
}

ezProcessingStream::DataType ezParticleStreamFactory_Size::GetStreamDataType() const
{
  return ezProcessingStream::DataType::Float;
}

void ezParticleStream_Size::InitializeElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements)
{
  ezProcessingStreamIterator<float> itData(m_pStream, uiNumElements, uiStartIndex);

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
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleStream_Color, 1, ezRTTIDefaultAllocator<ezParticleStream_Color>)
EZ_END_DYNAMIC_REFLECTED_TYPE

const ezRTTI* ezParticleStreamFactory_Color::GetParticleStreamType() const
{
  return ezGetStaticRTTI<ezParticleStream_Color>();
}

const char* ezParticleStreamFactory_Color::GetStreamName() const
{
  return "Color";
}

ezProcessingStream::DataType ezParticleStreamFactory_Color::GetStreamDataType() const
{
  return ezProcessingStream::DataType::Float4;
}

void ezParticleStream_Color::InitializeElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements)
{
  ezProcessingStreamIterator<ezColor> itData(m_pStream, uiNumElements, uiStartIndex);

  const ezColor defValue(1.0f, 1.0f, 1.0f, 1.0f);
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
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleStream_Velocity, 1, ezRTTIDefaultAllocator<ezParticleStream_Velocity>)
EZ_END_DYNAMIC_REFLECTED_TYPE

const ezRTTI* ezParticleStreamFactory_Velocity::GetParticleStreamType() const
{
  return ezGetStaticRTTI<ezParticleStream_Velocity>();
}

const char* ezParticleStreamFactory_Velocity::GetStreamName() const
{
  return "Velocity";
}

ezProcessingStream::DataType ezParticleStreamFactory_Velocity::GetStreamDataType() const
{
  return ezProcessingStream::DataType::Float3;
}

//void ezParticleStream_Velocity::InitializeElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements)
//{
//  ezProcessingStreamIterator<ezVec3> itData(m_pStream, uiNumElements);
//
//  const ezVec3 defValue(0.0f, 0.0f, 0.1f);
//  while (!itData.HasReachedEnd())
//  {
//    itData.Current() = defValue;
//    itData.Advance();
//  }
//}

//////////////////////////////////////////////////////////////////////////
// LAST POSITION STREAM
//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleStreamFactory_LastPosition, 1, ezRTTIDefaultAllocator<ezParticleStreamFactory_LastPosition>)
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleStream_LastPosition, 1, ezRTTIDefaultAllocator<ezParticleStream_LastPosition>)
EZ_END_DYNAMIC_REFLECTED_TYPE

const ezRTTI* ezParticleStreamFactory_LastPosition::GetParticleStreamType() const
{
  return ezGetStaticRTTI<ezParticleStream_LastPosition>();
}

const char* ezParticleStreamFactory_LastPosition::GetStreamName() const
{
  return "LastPosition";
}

ezProcessingStream::DataType ezParticleStreamFactory_LastPosition::GetStreamDataType() const
{
  return ezProcessingStream::DataType::Float3;
}

//////////////////////////////////////////////////////////////////////////
// ROTATION SPEED STREAM
//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleStreamFactory_RotationSpeed, 1, ezRTTIDefaultAllocator<ezParticleStreamFactory_RotationSpeed>)
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleStream_RotationSpeed, 1, ezRTTIDefaultAllocator<ezParticleStream_RotationSpeed>)
EZ_END_DYNAMIC_REFLECTED_TYPE

const ezRTTI* ezParticleStreamFactory_RotationSpeed::GetParticleStreamType() const
{
  return ezGetStaticRTTI<ezParticleStream_RotationSpeed>();
}

const char* ezParticleStreamFactory_RotationSpeed::GetStreamName() const
{
  return "RotationSpeed";
}

ezProcessingStream::DataType ezParticleStreamFactory_RotationSpeed::GetStreamDataType() const
{
  return ezProcessingStream::DataType::Float;
}

//////////////////////////////////////////////////////////////////////////
// EFFECT ID STREAM
//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleStreamFactory_EffectID, 1, ezRTTIDefaultAllocator<ezParticleStreamFactory_EffectID>)
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleStream_EffectID, 1, ezRTTIDefaultAllocator<ezParticleStream_EffectID>)
EZ_END_DYNAMIC_REFLECTED_TYPE

const ezRTTI* ezParticleStreamFactory_EffectID::GetParticleStreamType() const
{
  return ezGetStaticRTTI<ezParticleStream_EffectID>();
}

const char* ezParticleStreamFactory_EffectID::GetStreamName() const
{
  return "EffectID";
}

ezProcessingStream::DataType ezParticleStreamFactory_EffectID::GetStreamDataType() const
{
  return ezProcessingStream::DataType::Int;
}

//////////////////////////////////////////////////////////////////////////
// ON OFF STREAM
//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleStreamFactory_OnOff, 1, ezRTTIDefaultAllocator<ezParticleStreamFactory_OnOff>)
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleStream_OnOff, 1, ezRTTIDefaultAllocator<ezParticleStream_OnOff>)
EZ_END_DYNAMIC_REFLECTED_TYPE

const ezRTTI* ezParticleStreamFactory_OnOff::GetParticleStreamType() const
{
  return ezGetStaticRTTI<ezParticleStream_OnOff>();
}

const char* ezParticleStreamFactory_OnOff::GetStreamName() const
{
  return "OnOff";
}

ezProcessingStream::DataType ezParticleStreamFactory_OnOff::GetStreamDataType() const
{
  return ezProcessingStream::DataType::Int;
}

//////////////////////////////////////////////////////////////////////////
// AXIS STREAM
//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleStreamFactory_Axis, 1, ezRTTIDefaultAllocator<ezParticleStreamFactory_Axis>)
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleStream_Axis, 1, ezRTTIDefaultAllocator<ezParticleStream_Axis>)
EZ_END_DYNAMIC_REFLECTED_TYPE

const ezRTTI* ezParticleStreamFactory_Axis::GetParticleStreamType() const
{
  return ezGetStaticRTTI<ezParticleStream_Axis>();
}

const char* ezParticleStreamFactory_Axis::GetStreamName() const
{
  return "Axis";
}

ezProcessingStream::DataType ezParticleStreamFactory_Axis::GetStreamDataType() const
{
  return ezProcessingStream::DataType::Float3;
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
// AXIS STREAM
//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleStreamFactory_TrailData, 1, ezRTTIDefaultAllocator<ezParticleStreamFactory_TrailData>)
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleStream_TrailData, 1, ezRTTIDefaultAllocator<ezParticleStream_TrailData>)
EZ_END_DYNAMIC_REFLECTED_TYPE

const ezRTTI* ezParticleStreamFactory_TrailData::GetParticleStreamType() const
{
  return ezGetStaticRTTI<ezParticleStream_TrailData>();
}

const char* ezParticleStreamFactory_TrailData::GetStreamName() const
{
  return "TrailData";
}

ezProcessingStream::DataType ezParticleStreamFactory_TrailData::GetStreamDataType() const
{
  return ezProcessingStream::DataType::Int;
}
