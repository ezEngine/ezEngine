#pragma once

#include <ParticlePlugin/Basics.h>
#include <Foundation/Reflection/Implementation/DynamicRTTI.h>
#include <CoreUtils/DataProcessing/Stream/StreamProcessor.h>
#include <ParticlePlugin/Module/ParticleModule.h>

class EZ_PARTICLEPLUGIN_DLL ezParticleTypeFactory : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleTypeFactory, ezReflectedClass);

public:
  virtual const ezRTTI* GetTypeType() const = 0;
  virtual void CopyTypeProperties(ezParticleType* pObject) const = 0;

  ezParticleType* CreateType(ezParticleSystemInstance* pOwner) const;

  virtual void Save(ezStreamWriter& stream) const = 0;
  virtual void Load(ezStreamReader& stream) = 0;
};

class EZ_PARTICLEPLUGIN_DLL ezParticleType : public ezParticleModule<ezStreamProcessor, false>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleType, ezStreamProcessor);

  friend class ezParticleSystemInstance;

public:

  virtual void ExtractRenderData(const ezView& view, ezExtractedRenderData* pExtractedRenderData) const {}
  
  virtual void Render(const ezRenderViewContext& renderViewContext, ezRenderPipelinePass* pPass) const = 0;

private:

  
};


