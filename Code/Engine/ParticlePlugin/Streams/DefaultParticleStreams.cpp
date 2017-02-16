#include <PCH.h>
#include <ParticlePlugin/Streams/DefaultParticleStreams.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamIterator.h>

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

void ezParticleStream_Position::InitializeElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements)
{
  ezProcessingStreamIterator<ezVec3> itData(m_pStream, uiNumElements, uiStartIndex);

  const ezVec3 defValue = m_pOwner->GetTransform().m_vPosition;
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
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleStream_Size, 1, ezRTTIDefaultAllocator<ezParticleStream_Size>)
EZ_END_DYNAMIC_REFLECTED_TYPE

const ezRTTI* ezParticleStreamFactory_Size::GetParticleStreamType() const
{
  return ezGetStaticRTTI<ezParticleStream_Size>();
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



EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Streams_DefaultParticleStreams);

