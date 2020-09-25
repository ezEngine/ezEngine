#pragma once

#include <Foundation/DataProcessing/Stream/ProcessingStreamProcessor.h>
#include <Foundation/Reflection/Reflection.h>
#include <ParticlePlugin/Module/ParticleModule.h>
#include <ParticlePlugin/ParticlePluginDLL.h>

struct ezMsgExtractRenderData;

enum ezParticleTypeSortingKey
{
  Opaque = 0,
  BlendedBackground = 10,
  Additive = 20,
  BlendAdd = 30,
  Blended = 40,
  BlendedForeground = 50,
};

class EZ_PARTICLEPLUGIN_DLL ezParticleTypeFactory : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleTypeFactory, ezReflectedClass);

public:
  virtual const ezRTTI* GetTypeType() const = 0;
  virtual void CopyTypeProperties(ezParticleType* pObject, bool bFirstTime) const = 0;

  ezParticleType* CreateType(ezParticleSystemInstance* pOwner) const;

  virtual void QueryFinalizerDependencies(ezSet<const ezRTTI*>& inout_FinalizerDeps) const {}

  virtual void Save(ezStreamWriter& stream) const = 0;
  virtual void Load(ezStreamReader& stream) = 0;
};

class EZ_PARTICLEPLUGIN_DLL ezParticleType : public ezParticleModule
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleType, ezParticleModule);

  friend class ezParticleSystemInstance;

public:
  virtual float GetMaxParticleRadius(float fParticleSize) const { return fParticleSize * 0.5f; }

  virtual void ExtractTypeRenderData(ezMsgExtractRenderData& msg, const ezTransform& instanceTransform) const = 0;

protected:
  ezParticleType();

  virtual void InitializeElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements) override {}
  virtual void StepParticleSystem(const ezTime& tDiff, ezUInt32 uiNumNewParticles) { m_TimeDiff = tDiff; }

  static ezUInt32 ComputeSortingKey(ezParticleTypeRenderMode::Enum mode);

  ezTime m_TimeDiff;
  mutable ezUInt64 m_uiLastExtractedFrame;
};
