#pragma once

#include <PhysXPlugin/Components/PxCharacterShapeComponent.h>

struct ezPxCharacterCapsuleShapeData;

using ezPxCharacterCapsuleShapeComponentManager = ezComponentManager<class ezPxCharacterCapsuleShapeComponent, ezBlockStorageType::FreeList>;

class EZ_PHYSXPLUGIN_DLL ezPxCharacterCapsuleShapeComponent : public ezPxCharacterShapeComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezPxCharacterCapsuleShapeComponent, ezPxCharacterShapeComponent, ezPxCharacterCapsuleShapeComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;


protected:
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // ezPxCharacterCapsuleShapeComponent

public:
  ezPxCharacterCapsuleShapeComponent();
  ~ezPxCharacterCapsuleShapeComponent();

  virtual ezBitflags<ezPxCharacterShapeCollisionFlags> MoveShape(const ezVec3& vMoveDeltaGlobal) override; // [ scriptable ]

  virtual bool TestShapeSweep(ezPhysicsCastResult& out_sweepResult, const ezVec3& vDirGlobal, float fDistance) override;
  virtual bool TestShapeOverlap(const ezVec3& vGlobalFootPos, float fNewHeightValue) override;

  virtual float GetCurrentTotalHeight() override;

  float m_fCapsuleHeight = 1.0f;                               ///< [ property ] real character height is m_fCapsuleHeight + 2 * m_fCapsuleRadius
  float m_fCapsuleRadius = 0.25f;                              ///< [ property ] real character height is m_fCapsuleHeight + 2 * m_fCapsuleRadius

protected:
  void OnUpdateLocalBounds(ezMsgUpdateLocalBounds& msg) const; // [ msg handler ]

  ezUniquePtr<ezPxCharacterCapsuleShapeData> m_pData;
};
