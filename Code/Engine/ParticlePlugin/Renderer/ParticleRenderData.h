#pragma once

#include <ParticlePlugin/Basics.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <Foundation/Math/BoundingBoxSphere.h>

class ezParticleSystemInstance;

class EZ_PARTICLEPLUGIN_DLL ezParticleRenderData : public ezRenderData
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleRenderData, ezRenderData);

public:
  ezParticleRenderData();

  ezBoundingBoxSphere m_GlobalBounds;

  const ezParticleSystemInstance* m_pParticleSystem;
};

