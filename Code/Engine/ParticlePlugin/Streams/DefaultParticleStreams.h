#pragma once

#include <ParticlePlugin/Streams/ParticleStream.h>

//////////////////////////////////////////////////////////////////////////
// ZERO-INIT STREAM
//////////////////////////////////////////////////////////////////////////

class EZ_PARTICLEPLUGIN_DLL ezParticleStream_ZeroInit : public ezParticleStream
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleStream_ZeroInit, ezParticleStream);

protected:
  // base class implementation already zero fills the stream data
  // virtual void InitializeElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements) override;
};

//////////////////////////////////////////////////////////////////////////
// POSITION STREAM
//////////////////////////////////////////////////////////////////////////

class EZ_PARTICLEPLUGIN_DLL ezParticleStreamFactory_Position : public ezParticleStreamFactory
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleStreamFactory_Position, ezParticleStreamFactory);

public:
  ezParticleStreamFactory_Position();
};

class EZ_PARTICLEPLUGIN_DLL ezParticleStream_Position : public ezParticleStream
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleStream_Position, ezParticleStream);

protected:
  virtual void Initialize(ezParticleSystemInstance* pOwner) override;
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
  ezParticleStreamFactory_Size();
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
  ezParticleStreamFactory_Color();
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
  ezParticleStreamFactory_Velocity();
};

class EZ_PARTICLEPLUGIN_DLL ezParticleStream_Velocity : public ezParticleStream
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleStream_Velocity, ezParticleStream);

protected:
  virtual void Initialize(ezParticleSystemInstance* pOwner) override;
  virtual void InitializeElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements) override;

  ezParticleSystemInstance* m_pOwner;
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
  ezParticleStreamFactory_LastPosition();
};

//////////////////////////////////////////////////////////////////////////
// ROTATION SPEED STREAM
//////////////////////////////////////////////////////////////////////////

class EZ_PARTICLEPLUGIN_DLL ezParticleStreamFactory_RotationSpeed : public ezParticleStreamFactory
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleStreamFactory_RotationSpeed, ezParticleStreamFactory);

public:
  ezParticleStreamFactory_RotationSpeed();
};

//////////////////////////////////////////////////////////////////////////
// ROTATION OFFSET STREAM
//////////////////////////////////////////////////////////////////////////

class EZ_PARTICLEPLUGIN_DLL ezParticleStreamFactory_RotationOffset : public ezParticleStreamFactory
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleStreamFactory_RotationOffset, ezParticleStreamFactory);

public:
  ezParticleStreamFactory_RotationOffset();
};

//////////////////////////////////////////////////////////////////////////
// EFFECT ID STREAM
//////////////////////////////////////////////////////////////////////////

class EZ_PARTICLEPLUGIN_DLL ezParticleStreamFactory_EffectID : public ezParticleStreamFactory
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleStreamFactory_EffectID, ezParticleStreamFactory);

public:
  ezParticleStreamFactory_EffectID();
};

//////////////////////////////////////////////////////////////////////////
// ON OFF STREAM
//////////////////////////////////////////////////////////////////////////

class EZ_PARTICLEPLUGIN_DLL ezParticleStreamFactory_OnOff : public ezParticleStreamFactory
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleStreamFactory_OnOff, ezParticleStreamFactory);

public:
  ezParticleStreamFactory_OnOff();
};

//////////////////////////////////////////////////////////////////////////
// AXIS STREAM
//////////////////////////////////////////////////////////////////////////

class EZ_PARTICLEPLUGIN_DLL ezParticleStreamFactory_Axis : public ezParticleStreamFactory
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleStreamFactory_Axis, ezParticleStreamFactory);

public:
  ezParticleStreamFactory_Axis();
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
  ezParticleStreamFactory_TrailData();
};

//////////////////////////////////////////////////////////////////////////
// VARIATION STREAM
//////////////////////////////////////////////////////////////////////////

class EZ_PARTICLEPLUGIN_DLL ezParticleStreamFactory_Variation : public ezParticleStreamFactory
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleStreamFactory_Variation, ezParticleStreamFactory);

public:
  ezParticleStreamFactory_Variation();
};

class EZ_PARTICLEPLUGIN_DLL ezParticleStream_Variation : public ezParticleStream
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleStream_Variation, ezParticleStream);

protected:
  virtual void Initialize(ezParticleSystemInstance* pOwner) override;
  virtual void InitializeElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements) override;

  ezParticleSystemInstance* m_pOwner;
};
