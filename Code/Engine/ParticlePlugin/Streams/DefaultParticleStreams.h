#pragma once

#include <ParticlePlugin/Streams/ParticleStream.h>

//////////////////////////////////////////////////////////////////////////
// POSITION STREAM
//////////////////////////////////////////////////////////////////////////

class EZ_PARTICLEPLUGIN_DLL ezParticleStreamFactory_Position : public ezParticleStreamFactory
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleStreamFactory_Position, ezParticleStreamFactory);

public:
  virtual const ezRTTI* GetParticleStreamType() const override;
  virtual const char* GetStreamName() const { return "Position"; }
  virtual ezProcessingStream::DataType GetStreamDataType() const override { return ezProcessingStream::DataType::Float3; }
};

class EZ_PARTICLEPLUGIN_DLL ezParticleStream_Position : public ezParticleStream
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleStream_Position, ezParticleStream);

protected:
  virtual void Initialize(ezParticleSystemInstance* pOwner) override { m_pOwner = pOwner; }
  virtual void InitializeElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements) override;

  ezParticleSystemInstance* m_pOwner;
};

//////////////////////////////////////////////////////////////////////////
// SIZE STREAM
//////////////////////////////////////////////////////////////////////////

class EZ_PARTICLEPLUGIN_DLL ezParticleStreamFactory_Size : public ezParticleStreamFactory
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleStreamFactory_Size, ezParticleStreamFactory);

public:
  virtual const ezRTTI* GetParticleStreamType() const override;
  virtual const char* GetStreamName() const { return "Size"; }
  virtual ezProcessingStream::DataType GetStreamDataType() const override { return ezProcessingStream::DataType::Float; }
};

class EZ_PARTICLEPLUGIN_DLL ezParticleStream_Size : public ezParticleStream
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleStream_Size, ezParticleStream);

protected:
  virtual void InitializeElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements) override;
};

//////////////////////////////////////////////////////////////////////////
// COLOR STREAM
//////////////////////////////////////////////////////////////////////////

class EZ_PARTICLEPLUGIN_DLL ezParticleStreamFactory_Color : public ezParticleStreamFactory
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleStreamFactory_Color, ezParticleStreamFactory);

public:
  virtual const ezRTTI* GetParticleStreamType() const override;
  virtual const char* GetStreamName() const { return "Color"; }
  virtual ezProcessingStream::DataType GetStreamDataType() const override { return ezProcessingStream::DataType::Float4; }
};

class EZ_PARTICLEPLUGIN_DLL ezParticleStream_Color : public ezParticleStream
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleStream_Color, ezParticleStream);

protected:
  virtual void InitializeElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements) override;
};

//////////////////////////////////////////////////////////////////////////
// VELOCITY STREAM
//////////////////////////////////////////////////////////////////////////

class EZ_PARTICLEPLUGIN_DLL ezParticleStreamFactory_Velocity : public ezParticleStreamFactory
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleStreamFactory_Velocity, ezParticleStreamFactory);

public:
  virtual const ezRTTI* GetParticleStreamType() const override;
  virtual const char* GetStreamName() const { return "Velocity"; }
  virtual ezProcessingStream::DataType GetStreamDataType() const override { return ezProcessingStream::DataType::Float3; }
};

class EZ_PARTICLEPLUGIN_DLL ezParticleStream_Velocity : public ezParticleStream
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleStream_Velocity, ezParticleStream);

protected:

  //virtual void InitializeElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements) override;

};

//////////////////////////////////////////////////////////////////////////
// LIFETIME STREAM
//////////////////////////////////////////////////////////////////////////

// always default initialized by the behavior

//////////////////////////////////////////////////////////////////////////
// LAST POSITION STREAM
//////////////////////////////////////////////////////////////////////////

class EZ_PARTICLEPLUGIN_DLL ezParticleStreamFactory_LastPosition : public ezParticleStreamFactory
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleStreamFactory_LastPosition, ezParticleStreamFactory);

public:
  virtual const ezRTTI* GetParticleStreamType() const override;
  virtual const char* GetStreamName() const { return "LastPosition"; }
  virtual ezProcessingStream::DataType GetStreamDataType() const override { return ezProcessingStream::DataType::Float3; }
};

class EZ_PARTICLEPLUGIN_DLL ezParticleStream_LastPosition : public ezParticleStream
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleStream_LastPosition, ezParticleStream);

protected:
};

//////////////////////////////////////////////////////////////////////////
// ROTATION SPEED STREAM
//////////////////////////////////////////////////////////////////////////

class EZ_PARTICLEPLUGIN_DLL ezParticleStreamFactory_RotationSpeed : public ezParticleStreamFactory
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleStreamFactory_RotationSpeed, ezParticleStreamFactory);

public:
  virtual const ezRTTI* GetParticleStreamType() const override;
  virtual const char* GetStreamName() const { return "RotationSpeed"; }
  virtual ezProcessingStream::DataType GetStreamDataType() const override { return ezProcessingStream::DataType::Float; }
};

class EZ_PARTICLEPLUGIN_DLL ezParticleStream_RotationSpeed : public ezParticleStream
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleStream_RotationSpeed, ezParticleStream);

protected:
};

//////////////////////////////////////////////////////////////////////////
// EFFECT ID STREAM
//////////////////////////////////////////////////////////////////////////

class EZ_PARTICLEPLUGIN_DLL ezParticleStreamFactory_EffectID : public ezParticleStreamFactory
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleStreamFactory_EffectID, ezParticleStreamFactory);

public:
  virtual const ezRTTI* GetParticleStreamType() const override;
  virtual const char* GetStreamName() const { return "EffectID"; }
  virtual ezProcessingStream::DataType GetStreamDataType() const override { return ezProcessingStream::DataType::Int; }
};

class EZ_PARTICLEPLUGIN_DLL ezParticleStream_EffectID : public ezParticleStream
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleStream_EffectID, ezParticleStream);

protected:
};

//////////////////////////////////////////////////////////////////////////
// ON OFF STREAM
//////////////////////////////////////////////////////////////////////////

class EZ_PARTICLEPLUGIN_DLL ezParticleStreamFactory_OnOff : public ezParticleStreamFactory
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleStreamFactory_OnOff, ezParticleStreamFactory);

public:
  virtual const ezRTTI* GetParticleStreamType() const override;
  virtual const char* GetStreamName() const { return "OnOff"; }
  virtual ezProcessingStream::DataType GetStreamDataType() const override { return ezProcessingStream::DataType::Int; }
};

class EZ_PARTICLEPLUGIN_DLL ezParticleStream_OnOff : public ezParticleStream
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleStream_OnOff, ezParticleStream);

protected:
};

//////////////////////////////////////////////////////////////////////////
// AXIS STREAM
//////////////////////////////////////////////////////////////////////////

class EZ_PARTICLEPLUGIN_DLL ezParticleStreamFactory_Axis : public ezParticleStreamFactory
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleStreamFactory_Axis, ezParticleStreamFactory);

public:
  virtual const ezRTTI* GetParticleStreamType() const override;
  virtual const char* GetStreamName() const { return "Axis"; }
  virtual ezProcessingStream::DataType GetStreamDataType() const override { return ezProcessingStream::DataType::Float3; }
};

class EZ_PARTICLEPLUGIN_DLL ezParticleStream_Axis : public ezParticleStream
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleStream_Axis, ezParticleStream);

protected:
  virtual void InitializeElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements) override;
};

//////////////////////////////////////////////////////////////////////////
// TRAIL DATA STREAM
//////////////////////////////////////////////////////////////////////////

class EZ_PARTICLEPLUGIN_DLL ezParticleStreamFactory_TrailData : public ezParticleStreamFactory
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleStreamFactory_TrailData, ezParticleStreamFactory);

public:
  virtual const ezRTTI* GetParticleStreamType() const override;
  virtual const char* GetStreamName() const { return "TrailData"; }
  virtual ezProcessingStream::DataType GetStreamDataType() const override { return ezProcessingStream::DataType::Int; }
};

class EZ_PARTICLEPLUGIN_DLL ezParticleStream_TrailData : public ezParticleStream
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleStream_TrailData, ezParticleStream);

protected:
  //virtual void InitializeElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements) override;
};

