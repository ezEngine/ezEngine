#pragma once

#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <RmlUiPlugin/RmlUiInputState.h>

using ezRmlUiCanvas3DInteractionComponentManager = ezComponentManagerSimple<class ezRmlUiCanvas3DInteractionComponent, ezComponentUpdateType::WhenSimulating, ezBlockStorageType::Compact, ezWorldUpdatePhase::PostTransform>;

class ezCpuMeshResource;
class ezPhysicsWorldModuleInterface;

class EZ_RMLUIPLUGIN_DLL ezRmlUiCanvas3DInteractionComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezRmlUiCanvas3DInteractionComponent, ezComponent, ezRmlUiCanvas3DInteractionComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void OnSimulationStarted() override;

  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

public:
  ezRmlUiCanvas3DInteractionComponent();
  ~ezRmlUiCanvas3DInteractionComponent();

  void Update();
  void Interact(ezRmlUiInputState input, float fMaxDistance);

private:
  static bool RaycastMeshTexCoords(const ezCpuMeshResource* pMesh, const ezVec3& vRayOrigin, const ezVec3& vRayDir, ezVec2& out_vTexCoords, float fEpsilon = 0.00001f);

  ezUInt32 m_uiCollisionLayer = 0;
  ezPhysicsWorldModuleInterface* m_pPhysicsWorldModule = nullptr;
};
