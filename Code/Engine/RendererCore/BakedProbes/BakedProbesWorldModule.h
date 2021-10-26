#pragma once

#include <Core/Graphics/AmbientCubeBasis.h>
#include <Core/World/WorldModule.h>
#include <RendererCore/Declarations.h>

using ezProbeTreeSectorResourceHandle = ezTypedResourceHandle<class ezProbeTreeSectorResource>;

class EZ_RENDERERCORE_DLL ezBakedProbesWorldModule : public ezWorldModule
{
  EZ_DECLARE_WORLD_MODULE();
  EZ_ADD_DYNAMIC_REFLECTION(ezBakedProbesWorldModule, ezWorldModule);

public:
  ezBakedProbesWorldModule(ezWorld* pWorld);
  ~ezBakedProbesWorldModule();

  virtual void Initialize() override;
  virtual void Deinitialize() override;

  bool HasProbeData() const;

  struct ProbeIndexData
  {
    static constexpr ezUInt32 NumProbes = 8;
    ezUInt32 m_probeIndices[NumProbes];
    float m_probeWeights[NumProbes];
  };

  ezResult GetProbeIndexData(const ezVec3& globalPosition, const ezVec3& normal, ProbeIndexData& out_ProbeIndexData) const;

  ezAmbientCube<float> GetSkyVisibility(const ProbeIndexData& indexData) const;

private:
  friend class ezBakedProbesComponent;

  void SetProbeTreeResourcePrefix(const ezHashedString& prefix);

  ezProbeTreeSectorResourceHandle m_hProbeTree;
};
