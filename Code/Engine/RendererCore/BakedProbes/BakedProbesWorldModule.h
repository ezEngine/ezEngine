#pragma once

#include <Core/Graphics/AmbientCubeBasis.h>
#include <Core/World/WorldModule.h>
#include <RendererCore/Declarations.h>

class EZ_RENDERERCORE_DLL ezBakedProbesWorldModule : public ezWorldModule
{
  EZ_DECLARE_WORLD_MODULE();
  EZ_ADD_DYNAMIC_REFLECTION(ezBakedProbesWorldModule, ezWorldModule);

public:
  ezBakedProbesWorldModule(ezWorld* pWorld);
  ~ezBakedProbesWorldModule();

  virtual void Initialize() override;
  virtual void Deinitialize() override;

  struct ProbeIndexData
  {
  };

  ezResult GetProbeIndexData(const ezVec3& globalPosition, const ezVec3& normal, ProbeIndexData& out_ProbeIndexData) const;

  ezAmbientCube<float> GetSkyVisibility(const ProbeIndexData& indexData) const;
};
